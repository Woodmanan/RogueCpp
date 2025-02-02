#pragma once
#include <vector>
#include "Game/ThreadManagers.h"
#include "Data/RogueArena.h"
#include "Data/SaveManager.h"
#include "Debug/Debug.h"

using namespace std;

/*
 * Data manager for allocating objects in a save friendly way.
 * Data type register themselves, and registered data types have two 
 * components available - An arena allocator for permanent allocations,
 * and a hashmap for non-permanent allocations. Types that don't need to
 * serialize can also be placed into a generic bucket.
 * 
 * Paired with this is a handle type which stores their index and offset.
 * Handle = (signed char) + (3/4 uinteger). This allows for an invalid bit,
 * 127 registered types (type 0 reserved for generic), and 16,777,216 bytes of
 * allocations per type.
 * 
 * Handles should be kept very small if possible, but if necessary they could
 * easily be grown to a long integer for more types / allocations.
 * 
 * Note on serialization: The reasoning for these handles is to keep references
 * to objects based on a static offset inside of an arena, or a static int handle
 * in a hashmap. This lets game objects be stored in a save-friendly way, as 
 * recursive handles / chains of arbitrary depth can be serialized directly
 * without traversing the tree.
 */

/*
* Handle linking is a debug feature that lets handles store a pointer to their
* linked object. These links are evaluated lazily, in order to maintain invariants
* about when handles are actively pointing to an object.
*/
#ifdef _DEBUG
#define LINK_HANDLE
#endif

class Handle;
template<typename T>
class THandle;

class RogueDataManager
{
public:
	RogueDataManager();
	~RogueDataManager();

	template <typename T>
	int RegisterArena(int size)
	{
		int index = RogueSaveable<T>::ID;
		//SpecializedArena<T> test = SpecializedArena<T>(size);
		ASSERT(index == arenas.size());
		arenas.push_back(new SpecializedArena<T>(size));
		//arenas.emplace_back(SpecializedArena<T>(size));
		//arenas.push_back(SpecializedArena<T>(size));
		return index;
	}

	template <typename T>
	Handle AllocateGeneric()
	{
		int offset = arenas[0]->AllocateByOffset<T>();
		return Handle(0, offset);
	}

	template <typename T, class... Args>
	THandle<T> Allocate(Args&&... args)
	{
		int arenaNum = RogueSaveable<T>::ID;
		STRONG_ASSERT(arenaNum > 0);
		int offset = arenas[arenaNum]->AllocateByOffset<T>(std::forward<Args>(args)...);
		return THandle<T>(arenaNum, offset);
	}

	template <typename T>
	bool CanResolve(unsigned int offset)
	{
		return CanResolve(RogueSaveable<T>::ID, offset);
	}

	bool CanResolve(int index, unsigned int offset)
	{
		if (index > 0 && index < arenas.size())
		{
			return arenas[index]->Contains(offset);
		}
		return false;
	}

	void* ResolveHandle(int index, unsigned int offset)
	{
		return arenas[index]->Get<void>(offset);
	}

	template <typename T>
	T* ResolveHandle(int index, unsigned int offset)
	{
		return arenas[index]->Get<T>(offset);
	}

	template <typename T>
	T* ResolveByTypeIndex(unsigned int index)
	{
		int arenaNum = RogueSaveable<T>::ID;
		STRONG_ASSERT(arenaNum > 0);
		return arenas[arenaNum]->GetByTypeIndex<T>(index);
	}

	void SaveAll()
	{
		for (int i = 0; i < arenas.size(); i++)
		{
			arenas[i]->WriteInternals();
		}
	}

	void LoadAll()
	{
		for (int i = 0; i < arenas.size(); i++)
		{
			arenas[i]->ReadInternals();
		}
	}

private:
	vector<RogueArena*> arenas;
};

class Handle
{
	template <typename T> friend class THandle;

public:
	Handle()
	{
		_internalOffset = 0x80000000; //All 0's, first bit (invalid bit) set to 1
	}

	Handle(signed char index, unsigned int offset)
	{
		_internalOffset = (offset & 0x00FFFFFF) | (((int) index) << 24);
	}

	template <typename T>
	Handle(const THandle<T>& other)
	{
		_internalOffset = other._internalOffset;
	}

	bool IsValid() const //Check if 'valid' bit is set to true;
	{
		return !(_internalOffset & 0x80000000);
	}

	void* Get()
	{
#ifdef LINK_HANDLE
		RefreshLinkedObject();
#endif
		return GetDataManager()->ResolveHandle(GetIndex(), GetOffset());
	}

	signed char GetIndex() const { return (signed char) (_internalOffset >> 24) & (0xFF); }

	unsigned int GetOffset() const { return (_internalOffset & 0x00FFFFFF); }

	const unsigned int& GetInternalOffset() const { return _internalOffset; }
	void SetInternalOffset(unsigned int internalOffset) { _internalOffset = internalOffset; }

protected:
	unsigned int _internalOffset;

#ifdef LINK_HANDLE
	void* linked = nullptr;
	virtual void RefreshLinkedObject()
	{
		if (IsValid() && GetDataManager()->CanResolve(GetIndex(), GetOffset()))
		{
			linked = GetDataManager()->ResolveHandle(GetIndex(), GetOffset());
		}
		else
		{
			linked = nullptr;
		}
	}
#endif
};

template <typename T>
class THandle : public Handle
{
	friend class Handle;

public:
	THandle() : Handle() {}

	THandle(signed char index, unsigned int offset) : Handle(index, offset) {}

	THandle(const Handle& other)
	{
		_internalOffset = other._internalOffset;
	}

	template <typename T2>
	THandle(const THandle<T2>& other)
	{
		_internalOffset = other._internalOffset;
	}

	T* operator ->()
	{
#ifdef LINK_HANDLE
		RefreshLinkedObject();
#endif
		return GetDataManager()->ResolveHandle<T>(GetIndex(), GetOffset());
	}

	T& GetReference()
	{
#ifdef LINK_HANDLE
		RefreshLinkedObject();
#endif
		return *GetDataManager()->ResolveHandle<T>(GetIndex(), GetOffset());
	}

	friend bool operator<(const THandle& l, const THandle& r)
	{
		return l.GetInternalOffset() < r.GetInternalOffset();
	}

	friend bool operator==(const THandle& l, const THandle& r)
	{
		return l.GetInternalOffset() == r.GetInternalOffset();
	}

#ifdef LINK_HANDLE
protected:
	T* linked = nullptr;
	virtual void RefreshLinkedObject() override
	{
		if (IsValid() && GetDataManager()->CanResolve(GetIndex(), GetOffset()))
		{
			linked = GetDataManager()->ResolveHandle<T>(GetIndex(), GetOffset());
		}
		else
		{
			linked = nullptr;
		}
	}
#endif
};

template<typename> struct RegisterBase { };
template <typename T>
struct RegisterHelper
{
	RegisterHelper(int);
	static RegisterHelper<T> _helper;
};

template <typename T>
class RogueSaveable
{
public:
	virtual ~RogueSaveable() {}
	static int ID;

	int GetID()
	{
		return ID;
	}
};

#define REGISTER_SAVE_TYPE(index, ClassName)\
	class ClassName;\
	template<> int RogueSaveable<ClassName>::ID = index;
    //template<> RegisterHelper<ClassName>::RegisterHelper (int) { DEBUG_PRINT("%d: %s (Registered %d)", index, name, size); GetDataManager()->RegisterArena<ClassName>(size); }\
    template<> RegisterHelper<ClassName> RegisterHelper<ClassName>::_helper(size);

namespace Serialization
{
	template <typename Stream>
	void Serialize(Stream& stream, const Handle& value)
	{
		Write(stream, "Valid", value.IsValid());
		if (value.IsValid())
		{
			Write(stream, "offset", value.GetInternalOffset());
		}
	}

	template <typename Stream>
	void Deserialize(Stream& stream, Handle& value)
	{
		if (Read<Stream, bool>(stream, "Valid"))
		{
			unsigned int offset;
			Read(stream, "offset", offset);
			value.SetInternalOffset(offset);
			return;
		}
		value = Handle();
	}

	template <typename Stream, typename T>
	void Serialize(Stream& stream, const THandle<T>& value)
	{
		bool valid = value.IsValid();
		Write(stream, "Valid", valid);
		if (value.IsValid())
		{
			if constexpr (std::is_same<Stream, JSONStream>::value)
			{
				short typeIndex = value.GetIndex();
				unsigned int offset = value.GetOffset();
				Write(stream, "Type Index", typeIndex);
				Write(stream, "Offset", offset);
			}
			else
			{
				Write(stream, "offset", value.GetInternalOffset());
			}
		}
	}

	template <typename Stream, typename T>
	void Deserialize(Stream& stream, THandle<T>& value)
	{
		if (Read<Stream, bool>(stream, "Valid"))
		{
			if constexpr (std::is_same<Stream, JSONStream>::value)
			{
				short index;
				unsigned int offset;
				Read(stream, "Type Index", index);
				Read(stream, "Offset", offset);
				value = THandle<T>((unsigned char)index, offset);
			}
			else
			{
				unsigned int offset;
				Read(stream, "offset", offset);
				value.SetInternalOffset(offset);
			}
		}
		else
		{
			value = THandle<T>();
		}
	}
}