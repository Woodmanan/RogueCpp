#pragma once
#include <iostream>
#include <fstream>
#include <vector>
#include <filesystem>
#include <functional>
#include <memory>
#include <map>
#include <set>
#include <thread>
#include <shared_mutex>
#include <atomic>
#include <array>
#include "Debug/Debug.h"

//Resource management and loading system.

//Resources can be referenced by name or ID

//Pack functions are passed in to the system on INIT. On request, a resource is loaded. If found, packed version is returned 

class HashID
{
public:
	HashID()
	{
		value = 0;
	}

	HashID(const size_t& newValue)
	{
		value = newValue;
	}

	HashID(const std::string& name)
	{
		value = (size_t) std::hash<std::string>{}(name);
	}

	HashID(const char* name)
	{
		value = (size_t)std::hash<std::string>{}(name);
	}

	operator size_t() const { return value; }

	static HashID Mix(const HashID& lhs, const HashID& rhs)
	{
		size_t left = lhs;
		size_t right = rhs;
		return (left ^ (right << 1));
	}

private:
	size_t value;
};

const int resourceVersion = 1;

struct ResourceRequest
{
	HashID type;
	HashID name;
};

struct ResourceHandle
{
	int m_version;
	std::shared_ptr<void> m_ptr;
};

class ResourcePointer
{
public:
	ResourcePointer();
	ResourcePointer(HashID ID) : m_ID(ID) {}

	bool IsValid();
	bool IsReady();
	std::shared_ptr<void> GetResource();

	HashID m_ID;
};

template<typename T>
class TResourcePointer : public ResourcePointer
{
public:
	TResourcePointer() {}
	TResourcePointer(const ResourcePointer& other)
	{
		m_ID = other.m_ID;
	}

	std::shared_ptr<T> operator ->()
	{
		ASSERT(IsValid() && IsReady());
		return std::static_pointer_cast<T>(GetResource());
	}
};

//bool RequiresRepack(const HashID ID);
//void Pack_Internal();

//File saving shenanigans
struct ResourceHeader
{
	int version = resourceVersion;
	std::vector<HashID> dependencies;
};

struct PackContext
{
	std::filesystem::path source;
	std::filesystem::path destination;
	ResourceHeader header;
};

struct LoadContext
{
	std::filesystem::path source;
};

bool OpenReadPackFile(std::filesystem::path packed);
void OpenWritePackFile(std::filesystem::path path, ResourceHeader header);

class ResourceManager
{
public:
	ResourceManager();
	~ResourceManager();

	//Const values
	static const int numWorkerThreads = 4;

	static void InitResources();
	void Register(const HashID& type, std::function<void(PackContext&)> pack, std::function<std::shared_ptr<void>(LoadContext&)> load);
	void LaunchThreads();
	static void ShutdownResources();
	static ResourceManager* Get();

	ResourcePointer Load(const HashID& type, const HashID& name, PackContext* context = nullptr);
	ResourcePointer LoadSynchronous(const HashID& type, const HashID& name, PackContext* context = nullptr);
	std::vector<ResourcePointer> LoadFromConfig(const std::string type, const std::string tag, PackContext* context = nullptr);
	std::vector<ResourcePointer> LoadFromConfigSynchronous(const std::string type, const std::string tag, PackContext* context = nullptr);
	void _LoadAllFromConfig(const std::string& type, const std::string& tag, const std::filesystem::path& path, std::vector<ResourcePointer>& pointers, PackContext* context = nullptr);
	bool IsMatchingTag(const std::string& tag, const std::string& line);
	bool IsAnyTag(const std::string& line);

	template<typename T>
	TResourcePointer<T> Load(const HashID& type, const HashID& name)
	{
		return (TResourcePointer<T>) Load(type, name);
	}

	template<typename T>
	TResourcePointer<T> LoadSynchronous(const HashID& type, const HashID& name)
	{
		return (TResourcePointer<T>) LoadSynchronous(type, name);
	}

	//Resource Acquisitions
	bool IsResourceLoaded(const HashID& id);
	std::shared_ptr<void> GetResource(const HashID& id);
	void InsertLoadedResource(const HashID& id, const HashID& name, std::shared_ptr<void> ptr);

private:
	std::filesystem::path GetSource(const HashID& name);
	std::filesystem::path GetPacked(const HashID& ID);
	std::filesystem::path GetResources();

	bool IsNewer(std::filesystem::path current, std::filesystem::path dependency);
	bool RequiresPack(std::filesystem::path source, std::filesystem::path packed);

	bool AreDependenciesUpToDate(std::filesystem::path packed);

	void ThreadMainLoop();
	void CacheFileNames();

	//Requests
	bool HasResourceRequests();
	ResourceRequest PopResourceRequest();
	void Thread_LoadRequest(ResourceRequest request, int threadNum);
	int FindOpenWorkerThread();

	volatile bool m_active;
	std::thread m_resourceThread;
	static ResourceManager* manager;

	//Loaded resources
	std::shared_mutex resourceMutex;
	std::map<HashID, std::shared_ptr<void>> loadedResources;
	std::set<HashID> inProgress;

	//Loading pipeline
	std::mutex loadingMutex;
	std::condition_variable loadingCv;
	std::vector<ResourceRequest> resourceRequests;

	std::map<HashID, std::string> fileNames;
	std::map<HashID, std::function<void(PackContext&)>> packFunctions;
	std::map<HashID, std::function<std::shared_ptr<void>(LoadContext&)>> loadFunctions;

	std::array<std::thread, numWorkerThreads> workerThreads;
	std::array<std::atomic<bool>, numWorkerThreads> workerFlags;
	int numWorkers = 0;
};

namespace Serialization
{
	template<typename Stream>
	void Serialize(Stream& stream, HashID& value)
	{
		size_t asSize = value;
		Write(stream, "Value", asSize);
	}

	template<typename Stream>
	void Deserialize(Stream& stream, HashID& value)
	{
		size_t asSize;
		Read(stream, "Value", asSize);
		value = asSize;
	}

	template<typename Stream>
	void Serialize(Stream& stream, ResourceHeader& value)
	{
		Write(stream, "Resource Version", value.version);
		Write(stream, "Dependencies", value.dependencies);
	}

	template<typename Stream>
	void Deserialize(Stream& stream, ResourceHeader& value)
	{
		Read(stream, "Resource Version", value.version);
		Read(stream, "Dependencies", value.dependencies);
	}
}