#include "CoreDataTypes.h"
#include "Data/RogueDataManager.h"
#include "Data/SaveManager.h"
#include "Map/Map.h"
#include "Debug/Profiling.h"
#include "Game/ThreadManagers.h"

Vec3 Vec3::WrapPosition(Vec3 inPosition)
{
    return Vec3(
        ModulusNegative<short>(inPosition.x, LOCATION_MAX_X),
        ModulusNegative<short>(inPosition.y, LOCATION_MAX_Y),
        ModulusNegative<short>(inPosition.z, LOCATION_MAX_Z)
    );
}

Vec3 Vec3::WrapChunk(Vec3 inChunk)
{
    return Vec3(
        ModulusNegative<short>(inChunk.x, CHUNK_MAX_X),
        ModulusNegative<short>(inChunk.y, CHUNK_MAX_Y),
        ModulusNegative<short>(inChunk.z, CHUNK_MAX_Z)
    );
}

Location::Location()
{
#ifdef _DEBUG
    m_valid = false;
    m_x = 0;
    m_y = 0;
    m_z = 0;
#else
    m_data = invalidMask;
#endif

#ifdef LINK_TILE
    RefreshLinkedTile();
#endif
}

Location::Location(ushort x, ushort y, ushort z)
{
    static_assert((xSize + ySize + zSize + 1) == 32, "Location will not fit into an int!");
    ASSERT(x < (1 << xSize));
    ASSERT(y < (1 << ySize));
    ASSERT(z < (1 << zSize));
#ifdef _DEBUG
    m_x = x;
    m_y = y;
    m_z = z;
    m_valid = true;
#else
    m_data = ((x << (ySize + zSize)) & xMask) |
             ((y << (zSize))         & yMask) |
             ( z                     & zMask);
#endif

#ifdef LINK_TILE
    RefreshLinkedTile();
#endif
}

bool Location::GetValid() const
{
#ifdef _DEBUG
    return m_valid;
#else
    return !(m_data & invalidMask);
#endif
}

void Location::SetValid(bool valid)
{
#ifdef _DEBUG
    m_valid = valid;
#else
    if (valid)
    {
        m_data &= (~invalidMask);
    }
    else
    {
        m_data |= invalidMask;
    }
#endif
}

ushort Location::x() const
{
    ASSERT(GetValid());
#ifdef _DEBUG
    return m_x;
#else
    return (ushort)((m_data & xMask) >> (ySize + zSize));
#endif
}

ushort Location::y() const
{
    ASSERT(GetValid());
#ifdef _DEBUG
    return m_y;
#else
    return (ushort)((m_data & yMask) >> (zSize));
#endif
}

ushort Location::z() const
{
    ASSERT(GetValid());
#ifdef _DEBUG
        return m_z;
#else
    return (ushort)(m_data & zMask);
#endif
}

Vec3 Location::GetVector() const
{
    return Vec3(x(), y(), z());
}

void Location::SetVector(Vec3 vector)
{
    ASSERT(vector.x < (1 << xSize));
    ASSERT(vector.y < (1 << ySize));
    ASSERT(vector.z < (1 << zSize));
#ifdef _DEBUG
    m_x = vector.x;
    m_y = vector.y;
    m_z = vector.z;
    m_valid = true;
#else
    m_data = ((vector.x << (ySize + zSize)) & xMask) |
             ((vector.y << (zSize))         & yMask) |
             ( vector.z                     & zMask);
#endif
}

Vec2 Location::AsVec2()
{
    return Vec2(x(), y());
}

Vec3 Location::GetChunkPosition()
{
    return Vec3(x() / CHUNK_SIZE_X, y() / CHUNK_SIZE_Y, z() / CHUNK_SIZE_Z);
}

Vec3 Location::GetChunkLocalPosition()
{
    return Vec3(x() % CHUNK_SIZE_X, y() % CHUNK_SIZE_Y, z() % CHUNK_SIZE_Z);
}

Tile& Location::GetTile()
{
    //ASSERT(InMap());
    return GetDataManager()->ResolveByTypeIndex<ChunkMap>(0)->GetTile(*this);
}

Tile* Location::operator ->()
{
    //ASSERT(InMap());
    return &GetDataManager()->ResolveByTypeIndex<ChunkMap>(0)->GetTile(*this);
}

Location Location::GetNeighbor(Direction direction)
{
    Tile& tile = GetTile();
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
        Vec3 offset = VectorFromDirection(direction);
        return Location(Vec3::WrapPosition(GetVector() + offset));
    }
}

std::pair<Location, Direction> Location::Traverse(Vec2 offset, Direction rotation)
{
    return Traverse(offset.x, offset.y, rotation);
}

std::pair<Location, Direction> Location::Traverse(short xOffset, short yOffset, Direction rotation)
{
    ASSERT(xOffset >= -1 && xOffset <= 1);
    ASSERT(yOffset >= -1 && yOffset <= 1);

    if (xOffset == 0 && yOffset == 0)
    {
        return std::make_pair(Location(x(), y(), z()), North);
    }

    short index = (xOffset + 1) + ((1 - yOffset) * 3);

    /* 
    *  0 | 1 | 2
    *  3 | 4 | 5
    *  6 | 7 | 8
    */

    Direction direction = Direction::North;
    
    switch (index)
    {
        case 0:
            direction = Direction::NorthWest;
            break;
        case 1:
            direction = Direction::North;
            break;
        case 2:
            direction = Direction::NorthEast;
            break;
        case 3:
            direction = Direction::West;
            break;
        case 4:
            HALT();
            break;
        case 5:
            direction = Direction::East;
            break;
        case 6:
            direction = Direction::SouthWest;
            break;
        case 7:
            direction = Direction::South;
            break;
        case 8:
            direction = Direction::SouthEast;
            break;
    }

    return Traverse(direction, rotation);
}

std::pair<Location, Direction> Location::Traverse(Direction direction, Direction rotation)
{
    ROGUE_PROFILE_SECTION("LOS::Traverse");
    Direction finalDirection = Rotate(direction, rotation);

    if (!GetTile().m_stats.IsValid() || !GetTile().m_stats->m_neighbors.IsValid())
    {
        return _Traverse_No_Neighbor(finalDirection);
    }
    else
    {
        return _Traverse_Neighbors(finalDirection);
    }
}

std::pair<Location, Direction> Location::_Traverse_No_Neighbor(Direction direction)
{
    ROGUE_PROFILE_SECTION("LOS::_Traverse_No_Neighbor");
    ASSERT((!GetTile().m_stats.IsValid() || !GetTile().m_stats->m_neighbors.IsValid()));
    return std::make_pair(GetNeighbor(direction), North);
}

std::pair<Location, Direction> Location::_Traverse_Neighbors(Direction direction)
{
    ROGUE_PROFILE_SECTION("LOS::_Traverse_Neighbors");
    ASSERT(GetTile().m_stats.IsValid());
    THandle<TileNeighbors> neighbors = GetTile().m_stats->m_neighbors;
    ASSERT(neighbors.IsValid());

    Location foundTile;
    Direction foundDirection;
    switch (direction)
    {
        case North:
            foundTile = neighbors->N;
            foundDirection = neighbors->N_Direction;
            break;
        case NorthEast:
            foundTile = neighbors->NE;
            foundDirection = neighbors->NE_Direction;
            break;
        case East:
            foundTile = neighbors->E;
            foundDirection = neighbors->E_Direction;
            break;
        case SouthEast:
            foundTile = neighbors->SE;
            foundDirection = neighbors->SE_Direction;
            break;
        case South:
            foundTile = neighbors->S;
            foundDirection = neighbors->S_Direction;
            break;
        case SouthWest:
            foundTile = neighbors->SW;
            foundDirection = neighbors->SW_Direction;
            break;
        case West:
            foundTile = neighbors->W;
            foundDirection = neighbors->W_Direction;
            break;
        case NorthWest:
            foundTile = neighbors->NW;
            foundDirection = neighbors->NW_Direction;
            break;
    }

    if (!foundTile.GetValid())
    {
        return _Traverse_No_Neighbor(direction);
    }
    return std::make_pair(foundTile, foundDirection);
}

THandle<TileNeighbors> Location::GetNeighbors()
{
    if (GetValid())
    {
        THandle<TileStats> stats = GetTile().m_stats;
        if (stats.IsValid())
        {
            THandle<TileNeighbors> neighbors = stats->m_neighbors;
            if (neighbors.IsValid())
            {
                return neighbors;
            }
        }
    }
    return THandle<TileNeighbors>();
}

#ifdef LINK_TILE
void Location::RefreshLinkedTile()
{
    /*
    if (GetValid() && Game::dataManager->CanResolve<Map>(z()) && InMap())
    {
        linkedTile = &Game::dataManager->ResolveByTypeIndex<Map>(z())->GetTile(x(), y());
    }
    else
    {
        linkedTile = nullptr;
    }
    */
}
#endif