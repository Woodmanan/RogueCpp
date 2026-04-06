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
#include <mutex>
#include <condition_variable>
#include <shared_mutex>
#include <atomic>
#include <array>
#include "Debug/Debug.h"
#include "Data/Serialization/Serialization.h"

//Resource management and loading system.

//Resources can be referenced by name or ID

//Pack functions are passed in to the system on INIT. On request, a resource is loaded. If found, packed version is returned 

using uint = uint32_t;

namespace Resources
{
	static constexpr int resourceVersion = 2;

	enum class EResourceStatus
	{
		Unrequested,
		Requested, //Sometimes resources are asked to load, but we're waiting for someone else to do the work! E.G, we might spin up requests, but our thread beats the job system to it
		Loading,
		Ready,
		Failed
	};

	class HashID
	{
	public:
		HashID();
		HashID(const size_t& newValue);
		HashID(const std::string& name);
		HashID(const char* name);

		bool IsValid();
		operator size_t() const;
		static HashID Mix(const HashID& lhs, const HashID& rhs);

	private:
		size_t value = 0;
	};

	struct ResourceHeader
	{
		ResourceHeader() {}
		ResourceHeader(HashID type) : m_type(type) {}

		HashID m_type;
		int m_version = resourceVersion;
		std::vector<HashID> m_dependencies;
	};

	struct ResourceRequest
	{
		ResourceRequest() {}
		ResourceRequest(HashID name, HashID type) : m_name(name), m_type(type) {}

		HashID m_name;
		HashID m_type;

		operator HashID() const
		{
			return GetHash();
		}

		HashID GetHash() const
		{
			return HashID::Mix(m_type, m_name);
		}
	};

	class ResourcePointer
	{
	public:
		ResourcePointer() {}
		ResourcePointer(HashID ID) : m_ID(ID) {}
		ResourcePointer(HashID ID, std::shared_ptr<void> ptr) : m_ID(ID), m_ptr(ptr) {}

		bool IsValid();
		bool IsReady();
		bool IsFailed();

		const HashID& GetID() const { return m_ID; }

	protected:
		std::shared_ptr<void> GetResource();
		HashID m_ID;
		std::shared_ptr<void> m_ptr;

		template<typename T>
		friend class TResourcePointer;
		friend class Serialization::Serializer<ResourcePointer>;
	};

	template<typename T>
	class TResourcePointer : public ResourcePointer
	{
	public:
		TResourcePointer() {}
		TResourcePointer(const ResourcePointer& other)
		{
			m_ID = other.m_ID;
			m_ptr = other.m_ptr;
		}

		std::shared_ptr<T> operator ->()
		{
			ASSERT(IsValid() && IsReady());
			return std::static_pointer_cast<T>(GetResource());
		}

		friend class Serialization::Serializer<TResourcePointer<T>>;
	};	

	struct PackContext
	{
		std::filesystem::path source;
		std::filesystem::path destination;
		ResourceHeader m_header;
	};

	struct LoadContext
	{
		std::filesystem::path source;
	};

	//Core Setup
	void Initialize();
	void Shutdown();
	void Register(const HashID& type, std::function<void(PackContext&)> pack, std::function<std::shared_ptr<void>(LoadContext&)> load);	
	void OpenWritePackFile(const PackContext& context);
	void CloseWritePackFile();
	
	//Main Loading
	ResourcePointer Load(const HashID& type, const HashID& name);
	ResourcePointer Load(const ResourceRequest& request);
	ResourcePointer Load(const HashID& ID);
	std::vector<ResourcePointer> Load(const std::vector<ResourceRequest>& requests);
	ResourcePointer LoadSynchronous(const HashID& type, const HashID& name);
	ResourcePointer LoadSynchronous(const ResourceRequest& request);
	ResourcePointer LoadSynchronous(const HashID& ID);
	std::vector<ResourcePointer> LoadSynchronous(const std::vector<ResourceRequest>& requests);

	std::vector<ResourcePointer> LoadSynchronous(const std::vector<ResourceRequest>& requests);

	template <typename T>
	TResourcePointer<T> LoadSynchronous(const HashID& type, const HashID& name)
	{
		return (TResourcePointer<T>) LoadSynchronous(type, name);
	}

	template <typename T>
	TResourcePointer<T> LoadSynchronous(const ResourceRequest& request)
	{
		return (TResourcePointer<T>) LoadSynchronous(request);
	}

	std::vector<ResourcePointer> LoadFromConfig(const std::string type, const std::string tag);
	std::vector<ResourcePointer> LoadFromConfigSynchronous(const std::string type, const std::string tag);

	//Dependency Loading - for internal use by packers
	//Always synchronous!
	ResourcePointer _LoadDependency(const HashID& type, const HashID& name, PackContext& context);
	std::vector<ResourcePointer> _LoadDependencies(const std::vector<ResourceRequest>& dependencies, PackContext& context);
	std::vector<ResourcePointer> _LoadDependenciesFromConfig(const std::string type, const std::string tag, PackContext& context);

	//Engine control - code defined resources
	void SetResource(const HashID& type, const HashID& name, std::shared_ptr<void> ptr);
	void SetResource(const HashID& ID, std::shared_ptr<void> ptr);
}


namespace Serialization
{
	template<>
	struct Serializer<Resources::HashID> : ObjectSerializer<Resources::HashID>
	{
		template<typename Stream>
		static void Serialize(Stream& stream, const Resources::HashID& value)
		{
			size_t asSize = value;
			Write(stream, "Value", asSize);
		}

		template<typename Stream>
		static void Deserialize(Stream& stream, Resources::HashID& value)
		{
			size_t asSize;
			Read(stream, "Value", asSize);
			value = asSize;
		}
	};
	
	template<>
	struct Serializer<Resources::ResourceHeader> : ObjectSerializer<Resources::ResourceHeader>
	{
		template<typename Stream>
		static void Serialize(Stream& stream, const Resources::ResourceHeader& value)
		{
			Write(stream, "Type", value.m_type);
			Write(stream, "Resource Version", value.m_version);
			Write(stream, "Dependencies", value.m_dependencies);
		}

		template<typename Stream>
		static void Deserialize(Stream& stream, Resources::ResourceHeader& value)
		{
			Read(stream, "Type", value.m_type);
			Read(stream, "Resource Version", value.m_version);
			Read(stream, "Dependencies", value.m_dependencies);
		}
	};

	template<>
	struct Serializer<Resources::ResourceRequest> : ObjectSerializer<Resources::ResourceRequest>
	{
		template<typename Stream>
		static void Serialize(Stream& stream, const Resources::ResourceRequest& value)
		{
			Write(stream, "Name", value.m_name);
			Write(stream, "Type", value.m_type);
		}

		template<typename Stream>
		static void Deserialize(Stream& stream, Resources::ResourceRequest& value)
		{
			Read(stream, "Name", value.m_name);
			Read(stream, "Type", value.m_type);
		}
	};

	template<>
	struct Serializer<Resources::ResourcePointer> : ObjectSerializer<Resources::ResourcePointer>
	{
		template<typename Stream>
		static void Serialize(Stream& stream, const Resources::ResourcePointer& value)
		{
			Write(stream, "ID", value.m_ID);
		}

		template<typename Stream>
		static void Deserialize(Stream& stream, Resources::ResourcePointer& value)
		{
			Read(stream, "ID", value.m_ID);
			Resources::LoadSynchronous(value.GetID());
		}
	};

	template<typename T>
	struct Serializer<Resources::TResourcePointer<T>> : ObjectSerializer<Resources::TResourcePointer<T>>
	{
		template<typename Stream>
		static void Serialize(Stream& stream, const Resources::TResourcePointer<T>& value)
		{
			Write(stream, "ID", value.m_ID);
		}

		template<typename Stream>
		static void Deserialize(Stream& stream, Resources::TResourcePointer<T>& value)
		{
			Read(stream, "ID", value.m_ID);
			Resources::LoadSynchronous(value.GetID());
		}
	};
}
