#pragma once

#include "../Data/RogueDataManager.h"
#include "../Core/CoreDataTypes.h"
#include "..\..\libraries\BearLibTerminal\Include\C\BearLibTerminal.h"
#include <type_traits>

class BackingTile;
class TileStats;
class TileNeighbors;
class Tile;
class Location;

class BackingTile
{
public:
    BackingTile() {}
    BackingTile(char character, color_t foreground, color_t background, bool blocksVision, float movementCost) :
        m_renderCharacter(character), 
        m_foregroundColor(foreground), 
        m_backgroundColor(background), 
        m_blocksVision(blocksVision), 
        m_movementCost(movementCost)
    {}

	char m_renderCharacter;
    color_t m_foregroundColor;
    color_t m_backgroundColor;
	bool m_blocksVision;
	float m_movementCost;
};

class TileStats
{
public:
    THandle<TileNeighbors> m_neighbors;
};

enum EVisibility : char
{
	HIDDEN = 0,
	REVEALED = 1,
	VISIBLE = 2
};

class TileNeighbors
{
public:
    Location N;
    Direction N_Direction;
    Location NE;
    Direction NE_Direction;
    Location E;
    Direction E_Direction;
    Location SE;
    Direction SE_Direction;
    Location S;
    Direction S_Direction;
    Location SW;
    Direction SW_Direction;
    Location W;
    Direction W_Direction;
    Location NW;
    Direction NW_Direction;
};

class Tile
{
public:
	THandle<BackingTile> m_backingTile;
	THandle<TileStats> m_stats;
    THandle<int> m_standing;
	EVisibility m_visibility;
};

class Map
{
public:
    Map() {}
    Map(Vec2 size, int mapZ, int interleave = 2) : m_size(size), m_interleaveBits(interleave)
    {
        ASSERT(size.x % (1 << (interleave)) == 0);
        ASSERT(size.y % (1 << (interleave)) == 0);

        m_tiles.resize(size.x * size.y, Tile());
        z = mapZ;
    }

    Vec2 m_size;
    int m_interleaveBits;
    int z;

    vector<THandle<BackingTile>> m_backingTiles;
    vector<Tile> m_tiles;

    int IndexIntoMap(Vec2 location);
    int IndexIntoMap(short x, short y);

    Tile& GetTile(Vec2 location);
    Tile& GetTile(ushort x, ushort y);

    void SetTile(Vec2 location, int index);
    void SetTile(Vec2 location, THandle<BackingTile> tile);

    void FillTilesInc(Vec2 from, Vec2 to, int index);
    void FillTilesExc(Vec2 from, Vec2 to, int index);

    void WrapMapEdges();
    void WrapTile(Vec2 location);
    Location WrapVector(Vec2 location, int xOffset, int yOffset);

    int LinkBackingTile(THandle<BackingTile> tile);
    template<typename T, class... Args>
    int LinkBackingTile(Args&&... args)
    {
        static_assert(std::is_convertible<T*, BackingTile*>::value, "T must inherit from BackingTile");
        THandle<BackingTile> tile = RogueDataManager::Allocate<T>(std::forward<Args>(args)...);
        return LinkBackingTile(tile);
    }
};

namespace RogueSaveManager
{
    void Serialize(BackingTile& value);
    void Deserialize(BackingTile& value);

    void Serialize(TileStats& value);
    void Deserialize(TileStats& value);

    void Serialize(Map& value);
    void Deserialize(Map& value);

    void Serialize(Tile& value);
    void Deserialize(Tile& value);

    void Serialize(TileNeighbors& value);
    void Deserialize(TileNeighbors& value);
}