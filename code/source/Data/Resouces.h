#pragma once
#include <iostream>
#include <fstream>
#include <vector>
#include <filesystem>
#include <functional>
#include <memory>
#include <map>
#include "../Debug/Debug.h"

//Resource management and loading system.

//Resources can be referenced by name or ID

//Pack functions are passed in to the system on INIT. On request, a resource is loaded. If found, packed version is returned 

namespace RogueResources
{
	typedef size_t HashID;
	const int resourceVersion = 1;

	struct ResourceHandle
	{
		int m_version;
		std::shared_ptr<void> m_ptr;
	};

	class ResourcePointer
	{
	public:
		ResourcePointer();
		ResourcePointer(HashID ID, ResourceHandle handle) : m_ID(ID), m_handle(handle) {}

		bool IsValid();
		bool NeedsReload();
		void Reload();

		HashID m_ID;
		ResourceHandle m_handle;
	};

	template<typename T>
	class TResourcePointer : public ResourcePointer
	{
	public:
		TResourcePointer(const ResourcePointer& other)
		{
			m_ID = other.m_ID;
			m_handle = other.m_handle;
		}

		std::shared_ptr<T> operator ->()
		{
			ASSERT(IsValid());
			return std::static_pointer_cast<T>(m_handle.m_ptr);
		}
	};

	HashID GetHashID(const std::string type, const std::string name);

	std::filesystem::path GetSource(const std::string name);
	std::filesystem::path GetPacked(const HashID ID);
	std::filesystem::path GetResources();

	bool IsNewer(std::filesystem::path current, std::filesystem::path dependency);
	bool RequiresPack(std::filesystem::path source, std::filesystem::path packed);
	bool RequiresLoad(HashID ID);
	//bool RequiresRepack(const HashID ID);
	//void Pack_Internal();

	ResourcePointer Load(const std::string type, const std::string name);
	std::vector<ResourcePointer> LoadFromConfig(const std::string type, const std::string tag);
	void _LoadAllFromConfig(const std::string& type, const std::string& tag, const std::filesystem::path& path, std::vector<ResourcePointer>& pointers);
	bool IsMatchingTag(const std::string& tag, const std::string& line);
	bool IsAnyTag(const std::string& line);

	template<typename T>
	TResourcePointer<T> Load(const std::string type, const std::string name)
	{
		return (TResourcePointer<T>) Load(type, name);
	}

	struct PackContext
	{
		std::filesystem::path source;
		std::filesystem::path destination;
	};

	struct LoadContext
	{
		std::filesystem::path source;
	};

	std::map<std::string, std::function<void(PackContext&)>>& GetPackFunctions();
	std::map<std::string, std::function<std::shared_ptr<void>(LoadContext&)>>& GetLoadFunctions();
	std::map<HashID, ResourceHandle>& GetLoadedResources();

	void InsertNewLoadedResource(HashID ID, std::shared_ptr<void> resource);
	ResourcePointer GetLoadedResource(HashID ID);

	void Register(const std::string type, std::function<void(PackContext&)> pack, std::function<std::shared_ptr<void>(LoadContext&)> load);

	//File saving shenanigans
	struct ResourceHeader
	{
		int version = resourceVersion;
		std::vector<HashID> dependencies;
	};

	void LinkAsDependency(HashID hash);
	void OpenPackDependencies();
	void ClosePackDependencies();
	bool OpenReadPackFile(std::filesystem::path packed);
	void OpenWritePackFile(std::filesystem::path path);
	bool AreDependenciesUpToDate(std::filesystem::path packed);
}

namespace RogueSaveManager
{
	void Serialize(RogueResources::ResourceHeader& value);
	void Deserialize(RogueResources::ResourceHeader& value);
}