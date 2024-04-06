#include "RogueDataManager.h"

// initializing manager
RogueDataManager* RogueDataManager::manager = nullptr;

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