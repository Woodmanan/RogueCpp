#pragma once
#include <vector>
#include "RogueArena.h"
#include "SaveManager.h"

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

class Handle;
template<typename T>
class THandle;

class RogueDataManager
{

private:
	static RogueDataManager* manager;
	vector<RogueArena*> arenas;

	RogueDataManager()
	{
		//Create an insert generic arena
		arenas.push_back(new RogueArena(1024));
	}

	~RogueDataManager()
	{
		for (int i = 0; i < arenas.size(); i++)
		{
			delete(arenas[i]);
		}
	}


public:
	static RogueDataManager* Get()
	{
		if (manager == nullptr)
		{
			manager = new RogueDataManager();
		}

		return manager;
	}

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
	static Handle AllocateGeneric()
	{
		int offset = manager->arenas[0]->AllocateByOffset<T>();
		return Handle(0, offset);
	}

	template <typename T, class... Args>
	static THandle<T> Allocate(Args&&... args)
	{
		int arenaNum = RogueSaveable<T>::ID;
		STRONG_ASSERT(arenaNum > 0);
		int offset = manager->arenas[arenaNum]->AllocateByOffset<T>(std::forward<Args>(args)...);
		return THandle<T>(arenaNum, offset);
	}

	void* ResolveHandle(int index, unsigned int offset)
	{
		return manager->arenas[index]->Get<void>(offset);
	}

	template <typename T>
	T* ResolveHandle(int index, unsigned int offset)
	{
		return manager->arenas[index]->Get<T>(offset);
	}

	template <typename T>
	T* ResolveByTypeIndex(unsigned int index)
	{
		int arenaNum = RogueSaveable<T>::ID;
		STRONG_ASSERT(arenaNum > 0);
		return manager->arenas[arenaNum]->GetByTypeIndex<T>(index);
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

	bool IsValid() //Check if 'valid' bit is set to true;
	{
		return !(_internalOffset & 0x80000000);
	}

	void* Get()
	{
		return RogueDataManager::Get()->ResolveHandle(GetIndex(), GetOffset());
	}

	signed char GetIndex() { return (signed char) (_internalOffset >> 24) & (0xFF); }

	unsigned int GetOffset() { return (_internalOffset & 0x00FFFFFF); }

	unsigned int GetInternalOffset() { return _internalOffset; }
	void SetInternalOffset(unsigned int internalOffset) { _internalOffset = internalOffset; }

protected:
	unsigned int _internalOffset;
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
		return RogueDataManager::Get()->ResolveHandle<T>(GetIndex(), GetOffset());
	}

	T& GetReference()
	{
		return *RogueDataManager::Get()->ResolveHandle<T>(GetIndex(), GetOffset());
	}
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

#define REGISTER_SAVE_TYPE(index, ClassName, name, size)\
	class ClassName;\
	template<> int RogueSaveable<ClassName>::ID = index;\
    template<> RegisterHelper<ClassName>::RegisterHelper (int) { std::cout << index << ": "<< name << " Registered: " << std::to_string(size) << std::endl; RogueDataManager::Get()->RegisterArena<ClassName>(size); }\
    template<> RegisterHelper<ClassName> RegisterHelper<ClassName>::_helper(size);

namespace RogueSaveManager
{
	template <>
	void Serialize(Handle& value);

	template<>
	void Deserialize(Handle& value);

	template <typename T>
	void Serialize(THandle<T>& value)
	{
		AddOffset();
		Write("Valid", value.IsValid());
		if (value.IsValid())
		{
			if (debug)
			{
				Write("Type Index", (short)value.GetIndex());
				Write("Offset", value.GetOffset());
			}
			else
			{
				Write("offset", value.GetInternalOffset());
			}
		}
		RemoveOffset();
	}

	template<typename T>
	void Deserialize(THandle<T>& value)
	{
		AddOffset();
		if (Read<bool>("Valid"))
		{
			if (debug)
			{
				short index;
				unsigned int offset;
				Read("Type Index", index);
				Read("Offset", offset);
				value = THandle<T>((unsigned char)index, offset);
			}
			else
			{
				unsigned int offset;
				Read("offset", offset);
				value.SetInternalOffset(offset);
			}
		}
		else
		{
			value = THandle<T>();
		}
		RemoveOffset();
	}
}