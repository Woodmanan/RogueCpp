#include "Resources.h"
#include <filesystem>
#include "Utils/Utils.h"
#include "Debug/Profiling.h"
#include "Data/SaveManager.h"

ResourceManager* ResourceManager::manager = nullptr;

ResourcePointer::ResourcePointer()
{

}

bool ResourcePointer::IsValid()
{
	return (m_ID != 0);
}

bool ResourcePointer::IsReady()
{
	return IsValid() && ResourceManager::Get()->IsResourceLoaded(m_ID);
}

std::shared_ptr<void> ResourcePointer::GetResource()
{
	//TODO: make this work as well
	ASSERT(IsValid() && IsReady());
	return ResourceManager::Get()->GetResource(m_ID);
}

bool OpenReadPackFile(std::filesystem::path packed)
{
	if (RogueSaveManager::OpenReadSaveFileByPath(packed))
	{
		ResourceHeader header;
		RogueSaveManager::Read("Header", header);
		return true;
	}

	return false;
}

void OpenWritePackFile(std::filesystem::path path, ResourceHeader header)
{
	RogueSaveManager::OpenWriteSaveFileByPath(path);
	RogueSaveManager::Write("Header", header);
}

ResourceManager::ResourceManager()
{
	m_active = false;
}

ResourceManager::~ResourceManager()
{
	if (m_active)
	{
		m_active = false;
		_mm_mfence();
		m_resourceThread.join();
	}
}

void ResourceManager::InitResources()
{
	manager = new ResourceManager();
}

void ResourceManager::Register(const HashID& type, std::function<void(PackContext&)> pack, std::function<std::shared_ptr<void>(LoadContext&)> load)
{
	ASSERT(m_active == false); //This is not thread safe - needs to be done before the thread gets launched
	packFunctions[type] = pack;
	loadFunctions[type] = load;
}

void ResourceManager::LaunchThreads()
{
	ASSERT(m_active == false);
	m_active = true;
	_mm_mfence();
	m_resourceThread = std::thread(GetMember(this, &ResourceManager::ThreadMainLoop));
}

void ResourceManager::ShutdownResources()
{
	delete manager;
}

ResourceManager* ResourceManager::Get()
{
	ASSERT(manager != nullptr);
	return manager;
}

void ResourceManager::ThreadMainLoop()
{
	CacheFileNames();

	while (m_active)
	{
		ROGUE_PROFILE;
		if (HasResourceRequests())
		{
			int threadIndex = FindOpenWorkerThread();
			ResourceRequest nextRequest = PopResourceRequest();
			if ((size_t) nextRequest.type != 0)
			{
				ASSERT(workerFlags[threadIndex] == false);
				workerFlags[threadIndex] = true;
				_mm_mfence();
				workerThreads[threadIndex] = std::thread(GetMember(this, &ResourceManager::Thread_LoadRequest), nextRequest, threadIndex);
			}
		}
	}

	for (int i = 0; i < numWorkers; i++)
	{
		if (workerThreads[i].joinable())
		{
			workerThreads[i].join();
		}
	}

	return;
}

void ResourceManager::Thread_LoadRequest(ResourceRequest request, int index)
{
	ASSERT(workerFlags[index] == true);

	HashID ID = HashID::Mix(request.type, request.name);
	std::filesystem::path sourcePath = GetSource(request.name);
	std::filesystem::path packedPath = GetPacked(ID);
	bool repacked = false;

	if (!std::filesystem::exists(sourcePath) && !std::filesystem::exists(packedPath))
	{
		InsertLoadedResource(ID, nullptr);
		return;
	}

	if (RequiresPack(sourcePath, packedPath))
	{
		ROGUE_PROFILE_SECTION("Pack Resource");
		ASSERT(packFunctions.contains(request.type));

		DEBUG_PRINT("Packing from %s to %s", sourcePath.string().c_str(), packedPath.string().c_str());
		PackContext packContext = { sourcePath, packedPath };
		packFunctions[request.type](packContext);
		repacked = true;

		//If you hit this, it means you are modifying sourcepath in some way.
		ASSERT(!RequiresPack(sourcePath, packedPath));
	}

	if (repacked || !IsResourceLoaded(ID))
	{
		ROGUE_PROFILE_SECTION("Load From File");
		ASSERT(loadFunctions.contains(request.type));
		DEBUG_PRINT("Loading %s with handle %zu", sourcePath.string().c_str(), ID);
		LoadContext loadContext = { packedPath };
		std::shared_ptr<void> resource = loadFunctions[request.type](loadContext);

		InsertLoadedResource(ID, resource);
	}

	workerFlags[index] = false;
	_mm_mfence();
}

int ResourceManager::FindOpenWorkerThread()
{
	ASSERT(numWorkerThreads > 0 && numWorkers <= numWorkerThreads);
	while (true)
	{
		for (int i = 0; i < numWorkerThreads; i++)
		{
			//Easy out - if we're checking a fresh thread, go ahead and allocate it!
			if (i >= numWorkers)
			{
				numWorkers = i + 1;
				return i;
			}

			if (workerFlags[i] == false)
			{
				if (workerThreads[i].joinable())
				{
					workerThreads[i].join();
				}
				return i;
			}
		}
	}
}

void ResourceManager::CacheFileNames()
{
	std::filesystem::path resourceFolder = GetResources();
	for (const auto& p : std::filesystem::recursive_directory_iterator(resourceFolder)) {
		if (!std::filesystem::is_directory(p)) {
			HashID id = p.path().filename().string();
			ASSERT(!fileNames.contains(id));
			fileNames[id] = p.path().string();
		}
	}

	DEBUG_PRINT("Resource Thread loaded %d files", fileNames.size());
}

ResourcePointer ResourceManager::Load(const HashID& type, const HashID& name, PackContext* context)
{
	ROGUE_PROFILE_SECTION("Load Resource");
	HashID ID = HashID::Mix(type, name);

	if (context != nullptr)
	{
		context->header.dependencies.push_back(ID);
	}

	if (!IsResourceLoaded(ID))
	{
		PushResourceRequest({type, name});
	}

	return ResourcePointer(ID);
}

ResourcePointer ResourceManager::LoadSynchronous(const HashID& type, const HashID& name, PackContext* context)
{
	ResourcePointer ptr = Load(type, name, context);
	if (ptr.IsValid())
	{
		while (!ptr.IsReady())
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
	}

	return ptr;
}

std::vector<ResourcePointer> ResourceManager::LoadFromConfig(const std::string type, const std::string tag, PackContext* context)
{
	ROGUE_PROFILE_SECTION("Load From Config");
	std::vector<ResourcePointer> pointers = std::vector<ResourcePointer>();

	std::filesystem::path resourceFolder = GetResources();
	for (const auto& p : std::filesystem::recursive_directory_iterator(resourceFolder)) {
		if (!std::filesystem::is_directory(p)) {
			if (p.path().filename().string() == "config.ini")
			{
				_LoadAllFromConfig(type, tag, p.path(), pointers, context);
			}
		}
	}

	return pointers;
}

std::vector<ResourcePointer> ResourceManager::LoadFromConfigSynchronous(const std::string type, const std::string tag, PackContext* context)
{
	std::vector<ResourcePointer> pointers = LoadFromConfig(type, tag, context);
	for (ResourcePointer ptr : pointers)
	{
		if (ptr.IsValid())
		{
			while (!ptr.IsReady())
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(1));
			}
		}
	}

	return pointers;
}

void ResourceManager::_LoadAllFromConfig(const std::string& type, const std::string& tag, const std::filesystem::path& path, std::vector<ResourcePointer>& pointers, PackContext* context)
{
	std::ifstream stream;
	stream.open(path);

	if (!stream.is_open()) { return; }
	std::string line;
	bool packing = false;

	while (std::getline(stream, line))
	{
		if (line.empty()) { continue; }

		if (IsAnyTag(line))
		{
			if (IsMatchingTag(tag, line))
			{
				packing = true;
			}
			else
			{
				packing = false;
			}
		}
		else if (packing)
		{
			pointers.push_back(Load(type, line, context));
		}
	}
}

bool ResourceManager::IsMatchingTag(const std::string& tag, const std::string& line)
{
	ASSERT(IsAnyTag(line));
	if (tag.size() != line.size() - 2)
	{
		return false;
	}

	for (size_t i = 0; i < tag.size(); i++)
	{
		if (tag[i] != line[i + 1])
		{
			return false;
		}
	}

	return true;
}

bool ResourceManager::IsAnyTag(const std::string& line)
{
	if (line.empty()) { return false; }
	if (line.size() < 3) { return false; }
	if (line[0] != '[') { return false; }
	if (line[line.size() - 1] != ']') { return false; }

	return true;
}

bool ResourceManager::IsResourceLoaded(const HashID& id)
{
	std::shared_lock lock(resourceMutex);
	return loadedResources.contains(id);
}

std::shared_ptr<void> ResourceManager::GetResource(const HashID& id)
{
	std::shared_lock lock(resourceMutex);
	ASSERT(loadedResources.contains(id));
	return loadedResources[id];
}

void ResourceManager::InsertLoadedResource(const HashID& id, std::shared_ptr<void> ptr)
{
	std::unique_lock lock(resourceMutex);
	loadedResources[id] = ptr;
}

bool ResourceManager::HasResourceRequests()
{
	std::shared_lock lock(loadingMutex);
	return resourceRequests.size() > 0;
}

ResourceRequest ResourceManager::PopResourceRequest()
{
	std::unique_lock lock(loadingMutex);
	ResourceRequest request;
	if (resourceRequests.size() > 0)
	{
		request = resourceRequests[0];
		resourceRequests.erase(resourceRequests.begin());
	}
	
	return request;
}

void ResourceManager::PushResourceRequest(ResourceRequest request)
{
	std::unique_lock lock(loadingMutex);
	resourceRequests.push_back(request);
}

std::filesystem::path ResourceManager::GetSource(const HashID& name)
{
	if (fileNames.contains(name))
	{
		return fileNames[name];
	}
		
	return std::filesystem::path();
}

std::filesystem::path ResourceManager::GetPacked(const HashID& ID)
{
	std::filesystem::create_directory("./packed");
	return std::filesystem::path(string_format("./packed/%zu.pck", (size_t) ID));
}

std::filesystem::path ResourceManager::GetResources()
{
#ifdef _DEBUG
	return std::filesystem::path("./../../../resources");
#else
	return std::filesystem::path("./resources");
#endif
}

bool ResourceManager::IsNewer(std::filesystem::path current, std::filesystem::path dependency)
{
	return std::filesystem::last_write_time(current) < std::filesystem::last_write_time(dependency);
}

bool ResourceManager::RequiresPack(std::filesystem::path source, std::filesystem::path packed)
{
	ROGUE_PROFILE_SECTION("Check Requires Pack");

	if (!std::filesystem::exists(source)) { return false; }

	if (!std::filesystem::exists(packed)) { return true; }

	if (IsNewer(packed, source))
	{
		return true;
	}

	return !AreDependenciesUpToDate(packed);
}

bool ResourceManager::AreDependenciesUpToDate(std::filesystem::path packed)
{
	ROGUE_PROFILE_SECTION("Check Dependency Status");
	if (RogueSaveManager::OpenReadSaveFileByPath(packed))
	{
		ResourceHeader header;
		RogueSaveManager::Read("Header", header);
		RogueSaveManager::CloseReadSaveFile();

		if (header.version != resourceVersion)
		{
			return false;
		}

		for (HashID ID : header.dependencies)
		{
			if (IsNewer(packed, GetPacked(ID)))
			{
				return false;
			}
		}
	}

	return true;
}

namespace RogueSaveManager
{
	void Serialize(HashID& value)
	{
		AddOffset();
		size_t asSize = value;
		Write("Value", asSize);
		RemoveOffset();
	}

	void Deserialize(HashID& value)
	{
		AddOffset();
		size_t asSize;
		Read("Value", asSize);
		value = asSize;
		RemoveOffset();
	}

	void Serialize(ResourceHeader& value)
	{
		AddOffset();
		Write("Resource Version", value.version);
		Write("Dependencies", value.dependencies);
		RemoveOffset();
	}

	void Deserialize(ResourceHeader& value)
	{
		AddOffset();
		Read("Resource Version", value.version);
		Read("Dependencies", value.dependencies);
		RemoveOffset();
	}
}