#pragma once
#include "Data/SaveManager.h"
#include "Data/RogueDataManager.h"

REGISTER_SAVE_TYPE(1, BackingTile, "Backing Tile", 20);
REGISTER_SAVE_TYPE(2, TileStats, "Tile Stats", 2000);
REGISTER_SAVE_TYPE(3, Map, "Map", 30);
REGISTER_SAVE_TYPE(4, TileNeighbors, "Tile Neighbors", 100);
REGISTER_SAVE_TYPE(5, TileMemory, "Memory", 30);
REGISTER_SAVE_TYPE(6, MaterialContainer, "Material Container", 1000);