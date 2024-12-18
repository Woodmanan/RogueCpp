#pragma once

#include "Data/RogueDataManager.h"
#include "Core/CoreDataTypes.h"
#include "Core/Materials/Materials.h"
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
    BackingTile(char character, Color foreground, Color background, bool blocksVision, float movementCost, MaterialContainer materials) :
        m_renderCharacter(character),
        m_foregroundColor(foreground),
        m_backgroundColor(background),
        m_blocksVision(blocksVision),
        m_movementCost(movementCost),
        m_baseMaterials(materials)
    {}

	char m_renderCharacter;
    Color m_foregroundColor;
    Color m_backgroundColor;
	bool m_blocksVision;
	float m_movementCost;
    MaterialContainer m_baseMaterials;
    int m_index = -1;
};

class TileStats
{
public:
    THandle<TileNeighbors> m_neighbors;
    THandle<MaterialContainer> m_materialContainer;
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

    void Reset();

    THandle<TileStats> GetOrAddStats(Tile& tile);
    THandle<TileNeighbors> GetOrAddNeighbors(Tile& tile);
    void SetNeighbors(Tile& tile, TileNeighbors neighbors);

    void WrapMapEdges();
    void WrapTile(Vec2 location);
    Location WrapVector(Vec2 location, int xOffset, int yOffset);
    Location WrapLocation(Location location, int xOffset, int yOffset);
    void SetNeighbor(Vec2 location, Direction direction, Location neighbor, Direction rotation = North);
    Location GetNeighbor(Vec2 location, Direction direction);

    void CreatePortal(Vec2 open, Vec2 exit);
    void CreateDirectionalPortal(Vec2 open, Vec2 exit, Direction direction);
    void CreateBidirectionalPortal(Vec2 open, Direction openDir, Vec2 exit, Direction exitDir);

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