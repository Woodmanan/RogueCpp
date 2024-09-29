#include "Resouces.h"
#include <filesystem>
#include "../Utils/Utils.h"
#include "../Debug/Profiling.h"
#include "../Data/SaveManager.h"

namespace RogueResources
{
	std::map<std::string, std::function<void(PackContext&)>> packFunctions = std::map<std::string, std::function<void(PackContext&)>>();
	std::map<std::string, std::function<std::shared_ptr<void>(LoadContext&)>> loadFunctions = std::map<std::string, std::function<std::shared_ptr<void>(LoadContext&)>>();
	std::vector<ResourceHeader> contexts = std::vector<ResourceHeader>();

	ResourcePointer::ResourcePointer()
	{

	}

	bool ResourcePointer::IsValid()
	{
		return (m_ID != 0 && m_handle.m_ptr != nullptr);
	}

	bool ResourcePointer::NeedsReload()
	{
		if (!IsValid()) { return false; }

		ASSERT(GetLoadedResources().contains(m_ID));

		return GetLoadedResources()[m_ID].m_version != m_handle.m_version;
	}
	
	void ResourcePointer::Reload()
	{
		ASSERT(NeedsReload());
		m_handle = GetLoadedResources()[m_ID];
	}

	HashID GetHashID(const std::string type, const std::string name)
	{
		HashID typeHash = (HashID) std::hash<std::string>{}(type);
		HashID nameHash = (HashID) std::hash<std::string>{}(name);
		return typeHash ^ (nameHash << 1);
	}

	std::filesystem::path GetSource(const std::string name)
	{
		std::filesystem::path resourceFolder = GetResources();
		for (const auto& p : std::filesystem::recursive_directory_iterator(resourceFolder)) {
			if (!std::filesystem::is_directory(p)) {
				if (p.path().filename().string() == name)
				{
					return p.path();
				}
			}
		}

		return std::filesystem::path();
	}

	std::filesystem::path GetPacked(const HashID ID)
	{
		std::filesystem::create_directory("./packed");
		return std::filesystem::path(string_format("./packed/%zu.pck", ID));
	}

	std::filesystem::path GetResources()
	{
#ifdef _DEBUG
		return std::filesystem::path("./../../../resources");
#else
		return std::filesystem::path("./resources");
#endif
	}

	bool IsNewer(std::filesystem::path current, std::filesystem::path dependency)
	{
		return std::filesystem::last_write_time(current) < std::filesystem::last_write_time(dependency);
	}

	bool RequiresPack(std::filesystem::path source, std::filesystem::path packed)
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

	bool RequiresLoad(HashID ID)
	{
		return !GetLoadedResources().contains(ID);
	}

	ResourcePointer Load(const std::string type, const std::string name)
	{
		ROGUE_PROFILE_SECTION("Load Resource");
		HashID ID = GetHashID(type, name);
		LinkAsDependency(ID);

		std::filesystem::path sourcePath = GetSource(name);
		std::filesystem::path packedPath = GetPacked(ID);
		bool repacked = false;

		if (!std::filesystem::exists(sourcePath) && !std::filesystem::exists(packedPath))
		{
			return ResourcePointer();
		}

		if (RequiresPack(sourcePath, packedPath))
		{
			ROGUE_PROFILE_SECTION("Pack Resource");
			ASSERT(GetPackFunctions().contains(type));

			DEBUG_PRINT("Packing from %s to %s", sourcePath.string().c_str(), packedPath.string().c_str());
			OpenPackDependencies();
			PackContext packContext = { sourcePath, packedPath };
			GetPackFunctions()[type](packContext);
			repacked = true;
			ClosePackDependencies();

			//If you hit this, it means you are modifying sourcepath in some way.
			ASSERT(!RequiresPack(sourcePath, packedPath));
		}

		if (repacked || RequiresLoad(ID))
		{
			ROGUE_PROFILE_SECTION("Load From File");
			ASSERT(GetLoadFunctions().contains(type));
			DEBUG_PRINT("Loading %s as \"%s\" with handle %zu", name.c_str(), type.c_str(), ID);
			LoadContext loadContext = { packedPath };
			std::shared_ptr<void> resource = GetLoadFunctions()[type](loadContext);

			InsertNewLoadedResource(ID, resource);
		}

		return GetLoadedResource(ID);
	}

	std::vector<ResourcePointer> LoadFromConfig(const std::string type, const std::string tag)
	{
		ROGUE_PROFILE_SECTION("Load From Config");
		std::vector<ResourcePointer> pointers = std::vector<ResourcePointer>();

		std::filesystem::path resourceFolder = GetResources();
		for (const auto & p : std::filesystem::recursive_directory_iterator(resourceFolder)) {
			if (!std::filesystem::is_directory(p)) {
				if (p.path().filename().string() == "config.ini")
				{
					_LoadAllFromConfig(type, tag, p.path(), pointers);
				}
			}
		}

		return pointers;
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

	bool IsAnyTag(const std::string& line)
	{
		if (line.empty()) { return false; }
		if (line.size() < 3) { return false; }
		if (line[0] != '[') { return false; }
		if (line[line.size() - 1] != ']') { return false; }

		return true;
	}

	std::map<std::string, std::function<void(PackContext&)>>& GetPackFunctions()
	{
		static std::map<std::string, std::function<void(PackContext&)>> packFuncs;
		return packFuncs;
	}

	std::map<std::string, std::function<std::shared_ptr<void>(LoadContext&)>>& GetLoadFunctions()
	{
		static std::map<std::string, std::function<std::shared_ptr<void>(LoadContext&)>> loadFuncs;
		return loadFuncs;
	}

	std::map<HashID, ResourceHandle>& GetLoadedResources()
	{
		static std::map<HashID, ResourceHandle> loadedResources;
		return loadedResources;
	}

	void InsertNewLoadedResource(HashID ID, std::shared_ptr<void> resource)
	{
		ASSERT(resource != nullptr);

		int version = 0;
		if (GetLoadedResources().contains(ID))
		{
			version = GetLoadedResources()[ID].m_version + 1;
		}

		ResourceHandle handle;
		handle.m_ptr = resource;
		handle.m_version = version;

		GetLoadedResources()[ID] = handle;
		ASSERT(GetLoadedResources().contains(ID));
	}

	ResourcePointer GetLoadedResource(HashID ID)
	{
		ROGUE_PROFILE_SECTION("Get Preloaded Resource");
		ASSERT(GetLoadedResources().contains(ID));

		ResourcePointer pointer = ResourcePointer(ID, GetLoadedResources()[ID]);
		return pointer;
	}

	void Register(const std::string type, std::function<void(PackContext&)> pack, std::function<std::shared_ptr<void>(LoadContext&)> load)
	{
		GetPackFunctions()[type] = pack;
		GetLoadFunctions()[type] = load;
	}

	void LinkAsDependency(HashID hash)
	{
		if (contexts.size() > 0)
		{
			contexts[contexts.size() - 1].dependencies.push_back(hash);
		}
	}

	void OpenPackDependencies()
	{
		contexts.push_back(ResourceHeader());
	}

	void ClosePackDependencies()
	{
		contexts.pop_back();
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

	void OpenWritePackFile(std::filesystem::path path)
	{
		RogueSaveManager::OpenWriteSaveFileByPath(path);
		RogueSaveManager::Write("Header", contexts[contexts.size() - 1]);
	}

	bool AreDependenciesUpToDate(std::filesystem::path packed)
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
}

namespace RogueSaveManager
{
	void Serialize(RogueResources::ResourceHeader& value)
	{
		AddOffset();
		Write("Resource Version", value.version);
		Write("Dependencies", value.dependencies);
		RemoveOffset();
	}

	void Deserialize(RogueResources::ResourceHeader& value)
	{
		AddOffset();
		Read("Resource Version", value.version);
		Read("Dependencies", value.dependencies);
		RemoveOffset();
	}
}