#include "Resources.h"
#include <filesystem>
#include "Utils/Utils.h"
#include "Debug/Profiling.h"
#include "Data/SaveManager.h"
#include "Data/JobSystem.h"
#include <map>

namespace Resources
{
	struct ResourceData
	{
		EResourceStatus m_status;
		std::filesystem::file_time_type m_lastUpdate;
		std::shared_ptr<void> m_data;
	};

	//Static resource variables
	ROGUE_LOCK(std::mutex, resourceLock);
	std::map<HashID, std::string> fileNames;
	std::map<HashID, std::function<void(PackContext&)>> packFunctions;
	std::map<HashID, std::function<std::shared_ptr<void>(LoadContext&)>> loadFunctions;
	std::map<HashID, ResourceData> resourceData;
	std::map<HashID, ResourceRequest> requestData;

	std::condition_variable waitCondition;
	std::mutex waitMutex;

	//Predeclares
	EResourceStatus GetStatus(const HashID& resource);
	bool ResourceNeedsLoading(const HashID& resource);
	bool ResourceIsReady(const HashID& resource);
	bool ResourceIsFailed(const HashID& resource);
	std::shared_ptr<void> GetResourceData(const HashID& resource);
	void WaitForResource(const HashID& resource);

	void RequestResource(const HashID& resource);
	void LoadRequest(const HashID& ID);
	void LoadPackedResource(const HashID& ID);
	void PackIfUnclaimed(const HashID& ID);

	bool OpenReadPackFile(std::filesystem::path packed, ResourceHeader& header);

	bool HasSourceFile(const HashID& name);
	bool HasPackedFile(const HashID& ID);
	bool HasBlobContaining(const HashID& ID);
	std::filesystem::path GetSourceFile(const HashID& name);
	std::filesystem::path GetPackedFile(const HashID& name);

	bool IsPackedFileUpToDate(const HashID& ID);
	bool HasSourceChangedSincePack(const HashID& source, const HashID& packed);
	bool HasDependencyChangedSincePack(const HashID& dependency, const HashID& packed);
	void Pack(ResourceRequest& request);
	void LoadPackedFile(const HashID& ID);
	void LoadBlobFile(const HashID& ID);
	void MarkPackingFailure(const HashID& ID);

	std::filesystem::path GetExecutablePath();
	std::filesystem::path GetResources();
	std::filesystem::path FindResources();
	std::filesystem::path GetPackedFolder();

	void LoadCachedData();
	void WriteCachedData();

	HashID::HashID()
	{
		value = 0;
	}

	HashID::HashID(const size_t& newValue)
	{
		value = newValue;
	}

	HashID::HashID(const std::string& name)
	{
		value = (size_t) std::hash<std::string>{}(name);
	}

	HashID::HashID(const char* name)
	{
		value = (size_t)std::hash<std::string>{}(name);
	}

	bool HashID::IsValid()
	{
		return value != 0;
	}

	HashID::operator size_t() const
	{
		return value;
	}

	HashID HashID::Mix(const HashID& lhs, const HashID& rhs)
	{
		size_t left = lhs;
		size_t right = rhs;
		return (left ^ (right << 1));
	}

	bool ResourcePointer::IsValid()
	{
		return m_ID.IsValid();
	}

	bool ResourcePointer::IsReady()
	{
		if (m_ptr != nullptr)
		{
			return true;
		}

		return ResourceIsReady(m_ID);
	}

	bool ResourcePointer::IsFailed()
	{
		if (m_ptr != nullptr)
		{
			return false;
		}

		return ResourceIsFailed(m_ID);
	}

	std::shared_ptr<void> ResourcePointer::GetResource()
	{
		ASSERT(IsReady());

		if (m_ptr == nullptr)
		{
			m_ptr = GetResourceData(m_ID);
		}

		return m_ptr;
	}

	void Initialize()
	{
		std::filesystem::path resourceFolder = GetResources();
	
		LoadCachedData();

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

		DEBUG_PRINT("Resource System loaded %d files", fileNames.size());
	}

	void Shutdown()
	{
		WriteCachedData();
	}

	void Register(const HashID& type, std::function<void(PackContext&)> pack, std::function<std::shared_ptr<void>(LoadContext&)> load)
	{
		packFunctions[type] = pack;
		loadFunctions[type] = load;
	}

	bool OpenReadPackFile(std::filesystem::path packed, ResourceHeader& header)
	{
		ROGUE_PROFILE_SECTION("Open Read Pack File");
		if (RogueSaveManager::OpenReadSaveFileByPath(packed))
		{
			RogueSaveManager::Read("Header", header);
			return true;
		}

		return false;
	}

	void OpenWritePackFile(const PackContext& context)
	{
		RogueSaveManager::OpenWriteSaveFileByPath(context.destination);
		RogueSaveManager::Write("Header", context.m_header);
	}

	void CloseWritePackFile()
	{
		RogueSaveManager::CloseWriteSaveFile();
	}

	EResourceStatus GetStatus(const HashID& resource)
	{
		EResourceStatus status = EResourceStatus::Unrequested;
		resourceLock.lock();
		if (resourceData.contains(resource))
		{
			status = resourceData[resource].m_status;
		}
		resourceLock.unlock();
		return status;
	}

	bool ResourceNeedsLoading(const HashID& resource)
	{
		EResourceStatus status = GetStatus(resource);
		return status != EResourceStatus::Ready && status != EResourceStatus::Failed;
	}

	bool ResourceIsReady(const HashID& resource)
	{
		return GetStatus(resource) == EResourceStatus::Ready;
	}

	bool ResourceIsFailed(const HashID& resource)
	{
		return GetStatus(resource) == EResourceStatus::Failed;
	}

	std::shared_ptr<void> GetResourceData(const HashID& resource)
	{
		ASSERT(ResourceIsReady(resource));

		resourceLock.lock();
		std::shared_ptr<void> ptr = resourceData[resource].m_data;
		resourceLock.unlock();
		return ptr;
	}

	void WaitForResource(const HashID& resource)
	{
		ROGUE_PROFILE_SECTION("Wait For Resource");
		//We are waiting for this resource - might as well pack it ourselves if we can!
		PackIfUnclaimed(resource);

		if (ResourceNeedsLoading(resource))
		{
			DEBUG_PRINT("Waiting for %zu", resource);
			std::unique_lock<std::mutex> lock(waitMutex);
			waitCondition.wait(lock, [&resource]{ return !ResourceNeedsLoading(resource); });
		}
	}

	void RequestResource(const HashID& ID)
	{
		ROGUE_PROFILE_SECTION("Request Resource");

		resourceLock.lock();
		if (!resourceData.contains(ID))
		{
			//Insert empty data
			ResourceData data;
			data.m_status = EResourceStatus::Requested;
			resourceData[ID] = data;

			//Queue up a job to pack this at some point
			Jobs::QueueJob([ID]()
					{
					PackIfUnclaimed(ID);
					});
		}

		resourceLock.unlock();
	}

	void LoadRequest(const HashID& ID)
	{
		ROGUE_PROFILE_SECTION("Load Request");

		if (!IsPackedFileUpToDate(ID))
		{
			resourceLock.lock();
			ASSERT(requestData.contains(ID));
			ResourceRequest request = requestData[ID];
			resourceLock.unlock();
			Pack(request);
		}

		LoadPackedResource(ID);
	}

	void LoadPackedResource(const HashID& ID)
	{
		//Check if this file exists in a packed format
		if (HasPackedFile(ID))
		{
			LoadPackedFile(ID);
			return;
		}

		//Check if our blob directory has this file
		if (HasBlobContaining(ID))
		{
			LoadBlobFile(ID);
			return;
		}

		//Something has gone very wrong! Throw an error, and mark this as failed.
		MarkPackingFailure(ID);
	}

	void PackIfUnclaimed(const HashID& ID)
	{
		ROGUE_PROFILE_SECTION("Pack If Unclaimed");

		//Step 1: Check claim, and grab it if so!
		resourceLock.lock();
		ASSERT(resourceData.contains(ID));
		ResourceData& data = resourceData[ID];
		bool shouldPack = resourceData[ID].m_status == EResourceStatus::Requested;
		if (shouldPack)
		{
			resourceData[ID].m_status = EResourceStatus::Loading; //Very important - move it out of requested, so that we own it!
		}
		resourceLock.unlock();

		if (!shouldPack)
		{
			return;
		}

		LoadRequest(ID);
	}

	bool HasSourceFile(const HashID& name)
	{
		resourceLock.lock();
		bool hasSource = fileNames.contains(name) && std::filesystem::exists(fileNames[name]);
		resourceLock.unlock();
		return hasSource;
	}

	std::filesystem::path GetSourceFile(const HashID& name)
	{
		ASSERT(HasSourceFile(name));
		resourceLock.lock();
		std::filesystem::path path = fileNames[name];
		resourceLock.unlock();
		return path;
	}

	std::filesystem::path GetPackedFile(const HashID& ID)
	{
		std::filesystem::path packedPath = GetPackedFolder();
		std::filesystem::create_directory(packedPath);
		std::string fileName = string_format("%zu.pck", (size_t) ID);

		return packedPath / fileName;
	}


	bool HasPackedFile(const HashID& ID)
	{
		std::filesystem::path packed = GetPackedFile(ID);
		return std::filesystem::exists(packed);
	}

	bool HasBlobContaining(const HashID& ID)
	{
		DEBUG_PRINT("HasBlobContaining is unimplemented!!");
		return false;
	}	

	bool IsPackedFileUpToDate(const HashID& ID)
	{
		ROGUE_PROFILE_SECTION("IsPackedFileUpToDate");

		resourceLock.lock();
		ResourceRequest request = requestData.contains(ID) ? requestData[ID] : ResourceRequest();
		HashID name = request.m_name;
		resourceLock.unlock();

		if (!name.IsValid() || !HasSourceFile(name))
		{
			ASSERT(HasPackedFile(ID));
			return true;
		}

		if (!HasPackedFile(ID))
		{
			return false;
		}

		//First check - have we updated the source?
		if (HasSourceChangedSincePack(request.m_name, ID))
		{
			DEBUG_PRINT("Source file updated! Repacking");
			return true;
		}

		std::filesystem::path packedPath = GetPackedFile(ID);
	   	LoadContext loadContext = { packedPath };
		ResourceHeader header;

		//Quick check - load the header
		if (OpenReadPackFile(packedPath, header))
		{
			RogueSaveManager::CloseReadSaveFile();
			if (header.m_version != resourceVersion)
			{
				return false;
			}

			for (const ResourcePointer& ptr : header.m_dependencies)
			{
				if (!IsPackedFileUpToDate(ptr.GetID()) || HasDependencyChangedSincePack(ptr.GetID(), ID))
				{
					return false;
				}
			}
		}

		return true;
	}

	bool HasSourceChangedSincePack(const HashID& source, const HashID& packed)
	{
		ROGUE_PROFILE_SECTION("Has Source Changed");
		ASSERT(HasSourceFile(source));
		ASSERT(HasPackedFile(packed));

		return std::filesystem::last_write_time(GetSourceFile(source)) > std::filesystem::last_write_time(GetPackedFile(packed));
	}

	bool HasDependencyChangedSincePack(const HashID& dependency, const HashID& packed)
	{
		ROGUE_PROFILE_SECTION("Has Dependency Changed");
		ASSERT(HasPackedFile(dependency));
		ASSERT(HasPackedFile(packed));

		return std::filesystem::last_write_time(GetPackedFile(dependency)) > std::filesystem::last_write_time(GetPackedFile(packed));
	}

	void Pack(ResourceRequest& request)
	{
		ROGUE_PROFILE_SECTION("Pack");
		ASSERT(packFunctions.contains(request.m_type));

		std::filesystem::path sourcePath = GetSourceFile(request.m_name);
		std::filesystem::path packedPath = GetPackedFile(request);
		DEBUG_PRINT("Packing from %s to %s", sourcePath.string().c_str(), packedPath.string().c_str());
		ResourceHeader header = ResourceHeader(request.m_type);
		PackContext packContext = { sourcePath, packedPath, header };
		packFunctions[request.m_type](packContext);
	}

	void LoadPackedFile(const HashID& ID)
	{
		ROGUE_PROFILE_SECTION("Load Packed File");
		DEBUG_PRINT("Resources: Loading %zu", ID);
		std::filesystem::path packedPath = GetPackedFile(ID);
		LoadContext loadContext = { packedPath };
		ResourceHeader header;

		if (OpenReadPackFile(packedPath, header))
		{
			ASSERT(loadFunctions.contains(header.m_type));
			std::shared_ptr<void> resource = loadFunctions[header.m_type](loadContext);
			resourceLock.lock();
			resourceData[ID].m_data = resource;
			resourceData[ID].m_status = EResourceStatus::Ready;
			resourceLock.unlock();

			RogueSaveManager::CloseReadSaveFile();
		}


		DEBUG_PRINT("Resources: Finished loading %zu", ID);
		
		//Notify listeners that something loaded!
		waitCondition.notify_all();
	}

	void LoadBlobFile(const HashID& ID)
	{
		DEBUG_PRINT("Load blob file is unimplemented!!");

		//Notify listeners that something loaded!
		waitCondition.notify_all();
	}

	void MarkPackingFailure(const HashID& ID)
	{
		resourceLock.lock();
		resourceData[ID].m_status = EResourceStatus::Failed;
		resourceLock.unlock();

		//Notify listeners that something loaded!
		waitCondition.notify_all();
	}

	//Main Loading
	ResourcePointer Load(const HashID& ID)
	{
		ROGUE_PROFILE_SECTION("Load Resource");
		if (ResourceIsReady(ID))
		{
			return ResourcePointer(ID, GetResourceData(ID));
		}

		if (ResourceNeedsLoading(ID))
		{
			RequestResource(ID);
		}

		return ResourcePointer(ID);
	}

	ResourcePointer Load(const HashID& type, const HashID& name)
	{
		return Load(ResourceRequest(name, type));
	}

	ResourcePointer Load(const ResourceRequest& request)
	{
		HashID ID = request.GetHash();
		resourceLock.lock();
		requestData[ID] = request;
		resourceLock.unlock();

		return Load(ID);
	}

	std::vector<ResourcePointer> Load(const std::vector<ResourceRequest>& requests)
	{
		std::vector<ResourcePointer> outPointers;
		for (const ResourceRequest& request : requests)
		{
			outPointers.push_back(Load(request));
		}

		return outPointers;
	}

	ResourcePointer LoadSynchronous(const HashID& type, const HashID& name)
	{
		return LoadSynchronous(ResourceRequest(name, type));
	}

	ResourcePointer LoadSynchronous(const ResourceRequest& request)
	{
		ResourcePointer ptr = Load(request);
		WaitForResource(ptr.GetID());
		return ptr;
	}

	ResourcePointer LoadSynchronous(const HashID& ID)
	{
		ResourcePointer ptr = Load(ID);
		WaitForResource(ID);
		return ptr;
	}

	std::vector<ResourcePointer> LoadSynchronous(const std::vector<ResourceRequest>& requests)
	{
		std::vector<ResourcePointer> outPointers = Load(requests);
		for (ResourcePointer ptr : outPointers)
		{
			WaitForResource(ptr.GetID());
		}
		return outPointers;
	}

	bool IsAnyTag(const std::string& line)
	{
		if (line.empty()) { return false; }
		if (line.size() < 3) { return false; }
		if (line[0] != '[') { return false; }
		if (line[line.size() - 1] != ']') { return false; }

		return true;
	}

	bool IsMatchingTag(const std::string& tag, const std::string& line)
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

	void _LoadAllFromConfig(const std::string& type, const std::string& tag, const std::filesystem::path& path, std::vector<ResourcePointer>& pointers)
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
				pointers.push_back(Load(type, line));
			}
		}
	}

	std::vector<ResourcePointer> LoadFromConfig(const std::string type, const std::string tag)
	{
		ROGUE_PROFILE_SECTION("Load From Config");
		std::vector<ResourcePointer> pointers = std::vector<ResourcePointer>();

		std::filesystem::path resourceFolder = GetResources();
		for (const auto& p : std::filesystem::recursive_directory_iterator(resourceFolder)) {
			if (!std::filesystem::is_directory(p)) {
				if (p.path().filename().string() == "config.ini")
				{
					_LoadAllFromConfig(type, tag, p.path(), pointers);
				}
			}
		}

		return pointers;
	}

	std::vector<ResourcePointer> LoadFromConfigSynchronous(const std::string type, const std::string tag)
	{
		ROGUE_PROFILE_SECTION("Load From Config - Synchronous");
		std::vector<ResourcePointer> pointers = LoadFromConfig(type, tag);
		for (ResourcePointer ptr : pointers)
		{
			WaitForResource(ptr.GetID());
		}

		return pointers;
	}

	ResourcePointer _LoadDependency(const HashID& type, const HashID& name, PackContext& context)
	{
		ResourcePointer ptr = LoadSynchronous(type, name);
		context.m_header.m_dependencies.push_back(ptr.GetID());
		return LoadSynchronous(type, name);
	}

	std::vector<ResourcePointer> _LoadDependencies(const std::vector<ResourceRequest>& dependencies, PackContext& context)
	{
		std::vector<ResourcePointer> outPointers = LoadSynchronous(dependencies);

		for (const ResourcePointer& pointer : outPointers)
		{
			context.m_header.m_dependencies.push_back(pointer.GetID());
		}

		return outPointers;
	}

	std::vector<ResourcePointer> _LoadDependenciesFromConfig(const std::string type, const std::string tag, PackContext& context)
	{
		std::vector<ResourcePointer> outPointers = LoadFromConfigSynchronous(type, tag);

		for (const ResourcePointer& pointer : outPointers)
		{
			context.m_header.m_dependencies.push_back(pointer.GetID());
		}

		return outPointers;
	}

	std::filesystem::path GetExecutablePath()
	{
		//TODO: Check that this works on windows as well!
		return std::filesystem::canonical("/proc/self/exe");
	}

	std::filesystem::path GetResources()
	{
		static std::filesystem::path resourcePath = FindResources();
		return resourcePath;
	}

	std::filesystem::path FindResources()
	{
		DEBUG_PRINT("Finding resources!");
		int maxFoldersUp = 2;

		std::filesystem::path start = GetExecutablePath().parent_path();
		int layer = 0;
		do
		{	
			for (const auto& p : std::filesystem::recursive_directory_iterator(start))
			{
				if (std::filesystem::is_directory(p))
				{
					if (p.path().filename().string() == "resources")
					{
						DEBUG_PRINT("Resource folder found at %s", p.path().string().c_str());
						return p.path();
					}
				}
			}

			//TODO: Do this formatting in the realy way! This is gross.
			layer++;
			start = start.parent_path();

		} while (layer <= maxFoldersUp);

		DEBUG_PRINT("No resource folder found! Critical error, exiting...");
		HALT();
	}


	std::filesystem::path GetPackedFolder()
	{
		return GetExecutablePath().parent_path() / "packed";
	}

	void LoadCachedData()
	{
		if (RogueSaveManager::OpenReadSaveFile("RequestData.dat"))
		{
			RogueSaveManager::Read("RequestData", requestData);
			RogueSaveManager::CloseReadSaveFile();
			DEBUG_PRINT("Imported %d cached requests from disk", requestData.size());
		}
	}

	void WriteCachedData()
	{
		if (requestData.size() > 0)
		{
			RogueSaveManager::OpenWriteSaveFile("RequestData.dat");
			RogueSaveManager::Write("RequestData", requestData);
			RogueSaveManager::CloseWriteSaveFile();
			DEBUG_PRINT("Wrote %d cached requests to disk", requestData.size());
		}
	}

	void SetResource(const HashID& type, const HashID& name, std::shared_ptr<void> ptr)
	{
		ResourceRequest request = ResourceRequest(name, type);
		SetResource(request, ptr);
	}

	void SetResource(const HashID& ID, std::shared_ptr<void> ptr)
	{
		resourceLock.lock();
		resourceData[ID].m_data = ptr;
		resourceData[ID].m_status = EResourceStatus::Ready;
		resourceLock.unlock();
	}
}

/*
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
*/

