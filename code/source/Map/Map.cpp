#include "Map.h"

#define INTERLEAVE
static const unsigned int masks[] = { 0x00, 0x01, 0x03, 0x07, 0x0F, 0x1F, 0x3F, 0x7F, 0xFF };

int Map::IndexIntoMap(Vec2 location) const
{
    return IndexIntoMap(location.x, location.y);
}

int Map::IndexIntoMap(short x, short y) const
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

const Tile& Map::GetTile(Vec2 location) const
{
    return m_tiles[IndexIntoMap(location)];
}
const Tile& Map::GetTile(ushort x, ushort y) const
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

void Map::Reset()
{
    for (int i = 0; i < m_size.x; i++)
    {
        for (int j = 0; j < m_size.y; j++)
        {
            SetTile(Vec2(i, j), 0);
            Tile& tile = GetTile(i, j);
            if (tile.m_stats.IsValid())
            {
                WrapTile(Vec2(i, j));
            }
        }
    }
}

THandle<TileStats> Map::GetOrAddStats(Tile& tile)
{
    if (!tile.m_stats.IsValid())
    {
        tile.m_stats = GetDataManager()->Allocate<TileStats>();
    }

    ASSERT(tile.m_stats.IsValid());

    return tile.m_stats;
}

THandle<TileNeighbors> Map::GetOrAddNeighbors(Tile& tile)
{
    THandle<TileStats> stats = GetOrAddStats(tile);

    if (!stats->m_neighbors.IsValid())
    {
        stats->m_neighbors = GetDataManager()->Allocate<TileNeighbors>();
    }

    ASSERT(stats->m_neighbors.IsValid());

    return stats->m_neighbors;
}

void Map::SetNeighbors(Tile& tile, TileNeighbors neighbors)
{
    THandle<TileNeighbors> tileNeighbors = GetOrAddNeighbors(tile);
    tileNeighbors.GetReference() = neighbors;
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
    THandle<TileNeighbors> neighbors = GetOrAddNeighbors(tile);
    ASSERT(tile.m_stats.IsValid() && neighbors.IsValid() && tile.m_stats->m_neighbors.GetInternalOffset() == neighbors.GetInternalOffset());

    neighbors->N  = WrapVector(location,  0,  1);
    neighbors->NE = WrapVector(location,  1,  1);
    neighbors->E  = WrapVector(location,  1,  0);
    neighbors->SE = WrapVector(location,  1, -1);
    neighbors->S  = WrapVector(location,  0, -1);
    neighbors->SW = WrapVector(location, -1, -1);
    neighbors->W  = WrapVector(location, -1,  0);
    neighbors->NW = WrapVector(location, -1,  1);

    neighbors->N_Direction = North;
    neighbors->NE_Direction = North;
    neighbors->E_Direction = North;
    neighbors->SE_Direction = North;
    neighbors->S_Direction = North;
    neighbors->SW_Direction = North;
    neighbors->W_Direction = North;
    neighbors->NW_Direction = North;
}

Location Map::WrapVector(Vec2 location, int xOffset, int yOffset)
{
    int x = location.x + xOffset;
    x = (x + m_size.x) % m_size.x;
    int y = location.y + yOffset;
    y = (y + m_size.y) % m_size.y;
    return Location(x, y, z);
}

Location Map::WrapLocation(Location location, int xOffset, int yOffset) {
    int x = location.x() + xOffset;
    x = (x + m_size.x) % m_size.x;
    int y = location.y() + yOffset;
    y = (y + m_size.y) % m_size.y;
    return Location(x, y, location.z());
}

void Map::SetNeighbor(Vec2 location, Direction direction, Location neighbor, Direction rotation)
{
    Tile& tile = GetTile(location);
    THandle<TileNeighbors> neighbors = GetOrAddNeighbors(tile);
    switch (direction)
    {
    case North:
        neighbors->N  = neighbor;
        neighbors->N_Direction = rotation;
        break;
    case NorthEast:
        neighbors->NE = neighbor;
        neighbors->NE_Direction = rotation;
        break;
    case East:
        neighbors->E  = neighbor;
        neighbors->E_Direction = rotation;
        break;
    case SouthEast:
        neighbors->SE = neighbor;
        neighbors->SE_Direction = rotation;
        break;
    case South:
        neighbors->S  = neighbor;
        neighbors->S_Direction = rotation;
        break;
    case SouthWest:
        neighbors->SW = neighbor;
        neighbors->SW_Direction = rotation;
        break;
    case West:
        neighbors->W  = neighbor;
        neighbors->W_Direction = rotation;
        break;
    case NorthWest:
        neighbors->NW = neighbor;
        neighbors->NW_Direction = rotation;
        break;
    }
}

Location Map::GetNeighbor(Vec2 location, Direction direction)
{
    Tile& tile = GetTile(location);
    THandle<TileNeighbors> neighbors = tile.m_stats.IsValid() ? tile.m_stats->m_neighbors : THandle<TileNeighbors>();

    if (neighbors.IsValid())
    {
        switch (direction)
        {
        case North:
            return neighbors->N;
        case NorthEast:
            return neighbors->NE;
        case East:
            return neighbors->E;
        case SouthEast:
            return neighbors->SE;
        case South:
            return neighbors->S;
        case SouthWest:
            return neighbors->SW;
        case West:
            return neighbors->W;
        case NorthWest:
            return neighbors->NW;
        }
    }
    else
    {
        Vec2 offset = VectorFromDirection(direction);
        return WrapVector(location, offset.x, offset.y);
    }
}

void Map::CreatePortal(Vec2 open, Vec2 exit)
{
    WrapTile(open);
    WrapTile(exit);

    Tile& openTile = GetTile(open);
    Tile& exitTile = GetTile(exit);
    
    THandle<TileStats> openStats = GetOrAddStats(openTile);
    THandle<TileStats> exitStats = GetOrAddStats(exitTile);
    
    THandle<TileNeighbors> hold = openStats->m_neighbors;
    openStats->m_neighbors = exitStats->m_neighbors;
    exitStats->m_neighbors = hold;
}

void Map::CreateDirectionalPortal(Vec2 open, Vec2 exit, Direction direction)
{
    //Normalize neighbors
    WrapTile(open);
    WrapTile(exit);

    Direction CW = TurnClockwise(direction);
    Direction CCW = TurnCounterClockwise(direction);

    SetNeighbor(open, CW, GetNeighbor(exit, CW));
    SetNeighbor(open, direction, GetNeighbor(exit, direction));
    SetNeighbor(open, CCW, GetNeighbor(exit, CCW));

    SetNeighbor(exit, Reverse(CW), GetNeighbor(open, Reverse(CW)));
    SetNeighbor(exit, Reverse(direction), GetNeighbor(open, Reverse(direction)));
    SetNeighbor(exit, Reverse(CCW), GetNeighbor(open, Reverse(CCW)));
}

void Map::CreateBidirectionalPortal(Vec2 open, Direction openDir, Vec2 exit, Direction exitDir)
{
    WrapTile(open);
    WrapTile(exit);

    Direction rotation = FindRotationBetween(openDir, exitDir);

    Location exitLoc = GetNeighbor(exit, exitDir);
    Location exitLocCL = GetNeighbor(exit, TurnClockwise(exitDir));
    Location exitLocCCL = GetNeighbor(exit, TurnCounterClockwise(exitDir));

    Direction reverseOpen = Reverse(openDir);
    Direction reverseExit = Reverse(exitDir);
    Location openLoc = GetNeighbor(open, reverseOpen);
    Location openLocCL = GetNeighbor(open, TurnClockwise(reverseOpen));
    Location openLocCCL = GetNeighbor(open, TurnCounterClockwise(reverseOpen));

    SetNeighbor(open, openDir, exitLoc, rotation);
    SetNeighbor(open, TurnClockwise(openDir), exitLocCL, rotation);
    SetNeighbor(open, TurnCounterClockwise(openDir), exitLocCCL, rotation);

    SetNeighbor(exit, reverseExit, openLoc, ReverseRotation(rotation));
    SetNeighbor(exit, TurnClockwise(reverseExit), openLocCL, ReverseRotation(rotation));
    SetNeighbor(exit, TurnCounterClockwise(reverseExit), openLocCCL, ReverseRotation(rotation));
}