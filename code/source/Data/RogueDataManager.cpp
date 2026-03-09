#include "Data/RogueDataManager.h"

class BackingTile;

void* Handle::Get()
{
#ifdef LINK_HANDLE
	RefreshLinkedObject();
#endif
	return GetDataManager()->ResolveHandle(GetIndex(), GetOffset());
}

#ifdef LINK_HANDLE
void Handle::RefreshLinkedObject();
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


RogueDataManager::RogueDataManager()
{
	//Create an insert generic arena
	arenas.push_back(new RogueArena(1024));
}

RogueDataManager::~RogueDataManager()
{
	for (int i = 0; i < arenas.size(); i++)
	{
		delete(arenas[i]);
	}
}
