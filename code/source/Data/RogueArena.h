#pragma once
#include "Debug/Debug.h"
#include "SaveManager.h"
#include <malloc.h>
#include <algorithm>

struct ArenaHeader
{
	int currentOffset;
	int size;
};

class RogueArena
{
public:
	RogueArena() {};

	RogueArena(int dataSize)
	{
		int actualSize = std::max(dataSize, 4) + sizeof(ArenaHeader);
		buffer = std::vector<char>(actualSize, '\0');
		InitializeHeader(actualSize);
		ASSERT(buffer.capacity() > 0);
		ASSERT(GetAvailableSize() == dataSize);
	}

	RogueArena(const RogueArena& original)
	{
		buffer = original.buffer;
	}

	virtual ~RogueArena()
	{

	}

	template <typename T>
	T* Allocate()
	{
		if (sizeof(T) > GetAvailableSize())
		{
			Resize((GetHeader()->size - sizeof(ArenaHeader)) * 2);
		}

		ASSERT(sizeof(T) <= GetAvailableSize());

		ArenaHeader* header = GetHeader();
		T* pointer = (T*)&buffer[header->currentOffset];
		header->currentOffset += sizeof(T);
		*pointer = T();

		ASSERT(header->currentOffset <= header->size);

		return pointer;
	}

	template <typename T, class... Args>
	int AllocateByOffset(Args&&... args)
	{
		while (sizeof(T) > GetAvailableSize())
		{
			Resize((GetHeader()->size - sizeof(ArenaHeader)) * 2);
		}

		ASSERT(sizeof(T) <= GetAvailableSize());

		ArenaHeader* header = GetHeader();
		int offset = header->currentOffset;
		
		T* pointer = (T*)&buffer[header->currentOffset];
		header->currentOffset += sizeof(T);
		*pointer = T(std::forward<Args>(args)...);

		ASSERT(header->currentOffset <= header->size);

		return offset;
	}

	bool Contains(int offset)
	{
		offset += sizeof(ArenaHeader);
		ArenaHeader* header = GetHeader();
		return (offset >= 0 && offset < header->currentOffset);
	}

	template <typename T>
	T* Get(int offset)
	{
		ASSERT(offset < GetHeader()->currentOffset);
		return (T*)(&buffer[offset]);
	}

	template <typename T>
	T* GetByTypeIndex(int index)
	{
		int offset = sizeof(T) * index + sizeof(ArenaHeader);
		return Get<T>(offset);
	}

	void Clear()
	{
		GetHeader()->currentOffset = sizeof(ArenaHeader);
	}

	int GetAvailableSize()
	{
		ArenaHeader* header = GetHeader();
		return header->size - header->currentOffset;
	}

	void Resize(int newSize)
	{
		int realNewSize = newSize + sizeof(ArenaHeader);
		buffer.resize(realNewSize);
		GetHeader()->size = realNewSize;
	}

	virtual void WriteInternals()
	{
		RogueSaveManager::AddOffset();
		RogueSaveManager::Write("Buffer", buffer);
		RogueSaveManager::RemoveOffset();
	}

	virtual void ReadInternals()
	{
		RogueSaveManager::AddOffset();
		RogueSaveManager::Read("Buffer", buffer);
		RogueSaveManager::RemoveOffset();
	}

//protected:
	//char* bufferStart;
	std::vector<char> buffer;
protected:
	void InitializeHeader(int size)
	{
		ArenaHeader* header = (ArenaHeader*)&buffer[0];
		*header = ArenaHeader();
		header->size = size;
		header->currentOffset = sizeof(ArenaHeader);
	}

	ArenaHeader* GetHeader() { return (ArenaHeader*) &buffer[0]; }
};

template <typename T>
class SpecializedArena : public RogueArena
{
public:
	SpecializedArena(int dataSize) : RogueArena(dataSize)
	{

	}

	~SpecializedArena() override
	{
		for (int i = sizeof(ArenaHeader); i < GetHeader()->currentOffset; i += sizeof(T))
		{
			//TODO: Is this legal?? Find the right way to call the generic destructor.
			T* current = Get<T>(i);
			current->~T();
		}
		RogueArena::~RogueArena();
	}

	void WriteInternals() override
	{
		ArenaHeader* header = GetHeader();
		RogueSaveManager::AddOffset();
		RogueSaveManager::Write("size", header->size);
		RogueSaveManager::Write("offset", header->currentOffset);

		for (int i = sizeof(ArenaHeader); i < header->currentOffset; i += sizeof(T))
		{
			RogueSaveManager::Write("Value", *Get<T>(i));
		}
		RogueSaveManager::RemoveOffset();
	}

	void ReadInternals() override
	{
		RogueSaveManager::AddOffset();
		int size;
		RogueSaveManager::Read("size", size);
		buffer = vector<char>(size, '\0');
		InitializeHeader(size);
		ArenaHeader* header = GetHeader();
		RogueSaveManager::Read("offset", header->currentOffset);

		for (int i = sizeof(ArenaHeader); i < header->currentOffset; i += sizeof(T))
		{
			*Get<T>(i) = T();
			RogueSaveManager::Read("Value", *Get<T>(i));
		}
		RogueSaveManager::RemoveOffset();
	}
};

namespace RogueSaveManager
{
	void Serialize(RogueArena& arena);

	void Deserialize(RogueArena& arena);

	template <typename T>
	void Serialize(SpecializedArena<T>& arena)
	{
		arena.WriteInternals();
	}

	template <typename T>
	void Deserialize(SpecializedArena<T>& arena)
	{
		arena.ReadInternals();
	}
}