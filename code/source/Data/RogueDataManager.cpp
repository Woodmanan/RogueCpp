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

namespace RogueSaveManager
{
	void Serialize(Handle& value)
	{
		Write("Valid", value.IsValid());
		if (value.IsValid())
		{
			Write("offset", value.GetInternalOffset());
		}
	}

	void Deserialize(Handle& value)
	{
		if (Read<bool>("Valid"))
		{
			unsigned int offset;
			Read("offset", offset);
			value.SetInternalOffset(offset);
		}
		value = Handle();
	}
}