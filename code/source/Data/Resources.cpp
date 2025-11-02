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
	ROGUE_PROFILE_SECTION("Open Read Pack File");
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
		loadingCv.notify_all();
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

void ResourceManager::LaunchThreads(uint numThreads)
{
	ASSERT(m_active == false);
	numDedicatedWorkerThreads = std::min(numThreads, MaxWorkerThreads);
	m_active = true;
	_mm_mfence();
	m_resourceThread = std::thread(&ResourceManager::ThreadMainLoop, this);
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
	ROGUE_NAME_THREAD("Resource Manager");
	ROGUE_PROFILE;
	CacheFileNames();
	LaunchDedicatedWorkers();
	
	int count = 0;

	while (m_active)
	{
		//Hand over OS control of lock - keeps thread efficiently sleeping until someone wants to pass it info
		{
			std::unique_lock lock(loadingMutex);
			loadingCv.wait(lock, [this] { return !m_active || resourceRequests.size() > 0; });
		}

		while (HasResourceRequests())
		{
			ROGUE_PROFILE_SECTION("Process Resource Request");
			int threadIndex = FindOpenWorkerThread();

			if (threadIndex == -1)
			{
				continue;
			}

			ResourceRequest nextRequest = PopResourceRequest();
			if ((size_t) nextRequest.type != 0)
			{
				workerData[threadIndex].request = nextRequest;
				workerData[threadIndex].working = true;
				workerData[threadIndex].working.notify_all();
				_mm_mfence();
			}
		}
	}

	for (int i = 0; i < numDedicatedWorkerThreads; i++)
	{
		workerData[i].alive = false;
		workerData[i].working = true;
		workerData[i].working.notify_all();
		_mm_mfence();
	}

	for (int i = 0; i < numDedicatedWorkerThreads; i++)
	{
		ROGUE_PROFILE_SECTION("Close worker thread");
		workerData[i].thread.join();
	}

	return;
}

void ResourceManager::Thread_LoadRequest(ResourceRequest request, int index)
{
	ROGUE_PROFILE_SECTION("Thread Load Request");

	HashID ID = HashID::Mix(request.type, request.name);
	std::filesystem::path sourcePath = GetSource(request.name);
	std::filesystem::path packedPath = GetPacked(ID);
	bool repacked = false;

	{ //Quick check - is someone else working on this?
		std::unique_lock processLock(resourceMutex);
		if (inProgress.contains(ID) || loadedResources.contains(ID))
		{
			DEBUG_PRINT("Worker %d: Quitting load of '%s' - another thread has taken this task.", index, sourcePath.string().c_str());
			_mm_mfence();
			return;
		}

		inProgress.insert(ID);
	}

	{ //Secondary check - does someone have the source file we need?
		while (true)
		{
			std::unique_lock processLock(resourceMutex);
			if (!inProgress.contains(request.name))
			{
				inProgress.insert(request.name);
				break;
			}
		}
	}

	if (!std::filesystem::exists(sourcePath) && !std::filesystem::exists(packedPath))
	{
		InsertLoadedResource(ID, request.name, nullptr);
		_mm_mfence();
		return;
	}

	if (RequiresPack(sourcePath, packedPath))
	{
		ROGUE_PROFILE_SECTION("Pack Resource");
		ASSERT(packFunctions.contains(request.type));

		DEBUG_PRINT("Worker %d: Packing from %s to %s", index, sourcePath.string().c_str(), packedPath.string().c_str());
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
		DEBUG_PRINT("Worker %d: Loading %s with handle %zu", index, sourcePath.string().c_str(), ID);
		LoadContext loadContext = { packedPath };
		std::shared_ptr<void> resource = loadFunctions[request.type](loadContext);

		InsertLoadedResource(ID, request.name, resource);
	}

	_mm_mfence();
}

int ResourceManager::FindOpenWorkerThread()
{
	for (int i = 0; i < numDedicatedWorkerThreads; i++)
	{
		if (workerData[i].alive && !workerData[i].working)
		{
			return i;
		}
	}

	return -1;
}

void ResourceManager::CacheFileNames()
{
	std::filesystem::path resourceFolder = GetResources();
	for (const auto& p : std::filesystem::recursive_directory_iterator(resourceFolder)) {
		if (!std::filesystem::is_directory(p)) {
			HashID id = p.path().filename().string();

			if (p.path().filename().extension().string() == ".ini")
			{
				continue;
			}

			if (fileNames.contains(id))
			{
				DEBUG_PRINT("Matching file names: %s and %s", fileNames[id].c_str(), p.path().string().c_str());
				HALT();
			}
			fileNames[id] = p.path().string();
		}
	}

	DEBUG_PRINT("Resource Thread loaded %d files", fileNames.size());
}

void ResourceManager::LaunchDedicatedWorkers()
{
	for (int i = 0; i < numDedicatedWorkerThreads; i++)
	{
		workerData[i].alive = true;
		workerData[i].working = false;
		workerData[i].thread = std::thread(GetMember(this, &ResourceManager::WorkerThread), i);
	}
}

void ResourceManager::WorkerThread(int threadNum)
{
	std::string name = string_format("Resource Thread %d", threadNum);
	ROGUE_NAME_THREAD(name.c_str());
	while (workerData[threadNum].alive)
	{
		workerData[threadNum].working.wait(false);

		//Quick check on alive - out in case of death
		if (!workerData[threadNum].alive) { return; }

		//TODO: Wait for this to turn true in a better way
		if (workerData[threadNum].working)
		{
			Thread_LoadRequest(workerData[threadNum].request, threadNum);
			workerData[threadNum].working = false;
		}
	}
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
		{	
			std::lock_guard lock(loadingMutex);
			resourceRequests.push_back({ type, name });
		}
		loadingCv.notify_all();
	}

	return ResourcePointer(ID);
}

ResourcePointer ResourceManager::LoadSynchronous(const HashID& type, const HashID& name, PackContext* context)
{
	ROGUE_PROFILE_SECTION("Load Resource - Synchronous");
	ResourcePointer ptr = Load(type, name, context);
	if (ptr.IsValid())
	{
		ROGUE_PROFILE_SECTION("Waiting for load");
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
	ROGUE_PROFILE_SECTION("Load From Config - Synchronous");
	std::vector<ResourcePointer> pointers = LoadFromConfig(type, tag, context);
	for (ResourcePointer ptr : pointers)
	{
		if (ptr.IsValid())
		{
			ROGUE_PROFILE_SECTION("Waiting for loads");
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

void ResourceManager::InsertLoadedResource(const HashID& id, const HashID& name, std::shared_ptr<void> ptr)
{
	std::unique_lock lock(resourceMutex);
	loadedResources[id] = ptr;
	inProgress.erase(id);
	inProgress.erase(name);
}

bool ResourceManager::HasResourceRequests()
{
	std::lock_guard lock(loadingMutex);
	return resourceRequests.size() > 0;
}

ResourceRequest ResourceManager::PopResourceRequest()
{
	std::lock_guard lock(loadingMutex);
	ResourceRequest request;
	if (resourceRequests.size() > 0)
	{
		request = resourceRequests[0];
		resourceRequests.erase(resourceRequests.begin());
	}
	
	return request;
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