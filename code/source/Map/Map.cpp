#include "Map.h"

#define INTERLEAVE
static const unsigned int masks[] = { 0x00, 0x01, 0x03, 0x07, 0x0F, 0x1F, 0x3F, 0x7F, 0xFF };

int Map::IndexIntoMap(Vec2 location)
{
    return IndexIntoMap(location.x, location.y);
}

int Map::IndexIntoMap(short x, short y)
{
#ifdef INTERLEAVE
    int bigX = x >> m_interleaveBits;
    int bigY = y >> m_interleaveBits;
    int bigBlockIndex = (bigX + (m_size.x >> (m_interleaveBits)) * bigY) << (m_interleaveBits * 2);

    int lilX = x & masks[m_interleaveBits];
    int lilY = y & masks[m_interleaveBits];
    return bigBlockIndex + lilX + (1 << m_interleaveBits) * lilY;
#else // INTERLEAVE
	return (location.x + (m_size.x * location.y));
#endif
}



int Map::LinkBackingTile(THandle<BackingTile> tile)
{
	m_backingTiles.push_back(tile);
	ASSERT(m_backingTiles.size() > 0);
    tile->m_index = m_backingTiles.size() - 1;
	return m_backingTiles.size() - 1;
}

Tile& Map::GetTile(Vec2 location)
{
    return m_tiles[IndexIntoMap(location)];
}
Tile& Map::GetTile(ushort x, ushort y)
{
    return m_tiles[IndexIntoMap(x, y)];
}

void Map::SetTile(Vec2 location, int index)
{
	SetTile(location, m_backingTiles[index]);
}

void Map::SetTile(Vec2 location, THandle<BackingTile> tile)
{
	m_tiles[IndexIntoMap(location)].m_backingTile = tile;
}

void Map::FillTilesInc(Vec2 from, Vec2 to, int index)
{
    ASSERT(from.x <= to.x && from.y <= to.y);
    for (int i = from.x; i <= to.x; i++)
    {
        for (int j = from.y; j <= to.y; j++)
        {
            SetTile(Vec2(i, j), index);
        }
    }
}

void Map::FillTilesExc(Vec2 from, Vec2 to, int index)
{
    ASSERT(from.x < to.x && from.y < to.y);
    for (int i = from.x; i < to.x; i++)
    {
        for (int j = from.y; j < to.y; j++)
        {
            SetTile(Vec2(i, j), index);
        }
    }
}

void Map::WrapMapEdges()
{
    for (int x = 0; x < m_size.x; x++)
    {
        WrapTile(Vec2(x, 0));
        WrapTile(Vec2(x, m_size.y -1));
    }

    for (int y = 1; y < m_size.y - 1; y++)
    {
        WrapTile(Vec2(0, y));
        WrapTile(Vec2(m_size.x - 1, y));
    }
}

void Map::WrapTile(Vec2 location)
{
    Tile& tile = GetTile(location);
    if (!tile.m_stats.IsValid())
    {
        tile.m_stats = RogueDataManager::Allocate<TileStats>();
    }

    THandle<TileStats> stats = tile.m_stats;
    ASSERT(stats.IsValid());


    if (!tile.m_stats->m_neighbors.IsValid())
    {
        tile.m_stats->m_neighbors = RogueDataManager::Allocate<TileNeighbors>();
    }

    THandle<TileNeighbors> neighbors = tile.m_stats->m_neighbors;
    neighbors->N  = WrapVector(location,  0,  1);
    neighbors->NE = WrapVector(location,  1,  1);
    neighbors->E  = WrapVector(location,  1,  0);
    neighbors->SE = WrapVector(location,  1, -1);
    neighbors->S  = WrapVector(location,  0, -1);
    neighbors->SW = WrapVector(location, -1, -1);
    neighbors->W  = WrapVector(location, -1,  0);
    neighbors->NW = WrapVector(location, -1,  1);
}

Location Map::WrapVector(Vec2 location, int xOffset, int yOffset)
{
    int x = location.x + xOffset;
    x = (x + m_size.x) % m_size.x;
    int y = location.y + yOffset;
    y = (y + m_size.y) % m_size.y;
    return Location(x, y, z);
}

namespace RogueSaveManager
{
	void Serialize(BackingTile& value)
	{
		AddOffset();
		Write("Char", value.m_renderCharacter);
        Write("FGColor", value.m_foregroundColor);
        Write("BGColor", value.m_backgroundColor);
		Write("Blocks Vision", value.m_blocksVision);
		Write("Movement Cost", value.m_movementCost);
        Write("Index", value.m_index);
		RemoveOffset();
	}

    void Deserialize(BackingTile& value)
    {
        AddOffset();
        Read("Char", value.m_renderCharacter);
        Read("FGColor", value.m_foregroundColor);
        Read("BGColor", value.m_backgroundColor);
        Read("Blocks Vision", value.m_blocksVision);
        Read("Movement Cost", value.m_movementCost);
        Read("Index", value.m_index);
        RemoveOffset();
    }

    void Serialize(TileStats& value)
    {
        AddOffset();
        Write("Neighbors", value.m_neighbors);
        RemoveOffset();
    }

    void Deserialize(TileStats& value)
    {
        AddOffset();
        Read("Neighbors", value.m_neighbors);
        RemoveOffset();
    }

    void Serialize(Map& value)
    {
        AddOffset();
        Write("Size", value.m_size);
        Write("Interleave", value.m_interleaveBits);
        Write("Backing Tiles", value.m_backingTiles);
        if (debug)
        {
            for (int j = 0; j < value.m_size.y; j++)
            {
                for (int i = 0; i < value.m_size.x; i++)
                {
                    Vec2 location(i, j);
                    Write("Location", location);
                    Write("Tile", value.GetTile(location));
                }
            }
        }
        else
        {
            Write("Tiles", value.m_tiles);
        }
        RemoveOffset();
    }

    void Deserialize(Map& value)
    {
        AddOffset();
        Read("Size", value.m_size);
        Read("Interleave", value.m_interleaveBits);
        Read("Backing Tiles", value.m_backingTiles);
        value.m_tiles = std::vector<Tile>(value.m_size.x * value.m_size.y, Tile());
        if (debug)
        {
            for (int j = 0; j < value.m_size.y; j++)
            {
                for (int i = 0; i < value.m_size.x; i++)
                {
                    Vec2 location(i, j);
                    Read<Vec2>("Location");
                    Read("Tile", value.GetTile(location));
                }
            }
        }
        else
        {
            Read("Tiles", value.m_tiles);
        }
        RemoveOffset();
    }

    void Serialize(Tile& value)
    {
        AddOffset();
        Write("Backing", value.m_backingTile);
        Write("Stats", value.m_stats);
        if (debug)
        {
            Write("Visibility", (short)value.m_visibility);
        }
        else
        {
            Write("Visibility", (char)value.m_visibility);
        }
        RemoveOffset();
    }

    void Deserialize(Tile& value)
    {
        AddOffset();
        Read("Backing", value.m_backingTile);
        Read("Stats", value.m_stats);
        if (debug)
        {
            value.m_visibility = (EVisibility)Read<short>("Visibility");
        }
        else
        {
            value.m_visibility = (EVisibility)Read<char>("Visibility");
        }
        RemoveOffset();
    }

    void Serialize(TileNeighbors& value)
    {
        AddOffset();
        Write("N", value.N);
        Write("NDir", value.N_Direction);
        Write("NE", value.NE);
        Write("NEDir", value.NE_Direction);
        Write("E", value.E);
        Write("EDir", value.E_Direction);
        Write("SE", value.SE);
        Write("SEDir", value.SE_Direction);
        Write("S", value.S);
        Write("SDir", value.S_Direction);
        Write("SW", value.SW);
        Write("SWDir", value.SW_Direction);
        Write("W", value.W);
        Write("WDir", value.W_Direction);
        Write("NW", value.NW);
        Write("NWDir", value.NW_Direction);
        RemoveOffset();
    }

    void Deserialize(TileNeighbors& value)
    {
        AddOffset();
        Read("N", value.N);
        Read("NDir", value.N_Direction);
        Read("NE", value.NE);
        Read("NEDir", value.NE_Direction);
        Read("E", value.E);
        Read("EDir", value.E_Direction);
        Read("SE", value.SE);
        Read("SEDir", value.SE_Direction);
        Read("S", value.S);
        Read("SDir", value.S_Direction);
        Read("SW", value.SW);
        Read("SWDir", value.SW_Direction);
        Read("W", value.W);
        Read("WDir", value.W_Direction);
        Read("NW", value.NW);
        Read("NWDir", value.NW_Direction);
        RemoveOffset();
    }
}