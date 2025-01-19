#include "Data/RogueDataManager.h"

class BackingTile;

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