#pragma once
#include "SaveManager.h"
#include "RogueDataManager.h"

REGISTER_SAVE_TYPE(1, BackingTile, "Backing Tile", 20);
REGISTER_SAVE_TYPE(2, TileStats, "Tile Stats", 2000);
REGISTER_SAVE_TYPE(3, Map, "Map", 30);
REGISTER_SAVE_TYPE(4, TileNeighbors, "Tile Neighbors", 100);