#include "Data/RogueArena.h"

namespace RogueSaveManager
{
	void Serialize(RogueArena& arena)
	{
		arena.WriteInternals();
	}

	void Deserialize(RogueArena& arena)
	{
		arena.ReadInternals();
	}
}