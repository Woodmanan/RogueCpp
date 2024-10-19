#include "CoreDataTypes.h"
#include "Data/SaveManager.h"
#include "Map/Map.h"
#include "Debug/Profiling.h"

std::istream& operator >> (std::istream& in, Direction& direction)
{
    unsigned u = 0;
    in >> u;
    direction = static_cast<Direction>(u);
    return in;
}

std::ostream& operator << (std::ostream& out, Direction direction)
{
    unsigned u = direction;
    out << u;
    return out;
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

bool Location::GetValid()
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

ushort Location::x()
{
    ASSERT(GetValid());
#ifdef _DEBUG
    return m_x;
#else
    return (ushort)((m_data & xMask) >> (ySize + zSize));
#endif
}

ushort Location::y()
{
    ASSERT(GetValid());
#ifdef _DEBUG
    return m_y;
#else
    return (ushort)((m_data & yMask) >> (zSize));
#endif
}

ushort Location::z()
{
    ASSERT(GetValid());
#ifdef _DEBUG
        return m_z;
#else
    return (ushort)(m_data & zMask);
#endif
}

Vec3 Location::GetVector()
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

Tile& Location::GetTile()
{
    ASSERT(InMap());
    return RogueDataManager::Get()->ResolveByTypeIndex<Map>(z())->GetTile(x(), y());
}

Tile* Location::operator ->()
{
    ASSERT(InMap());
    return &RogueDataManager::Get()->ResolveByTypeIndex<Map>(z())->GetTile(x(), y());
}

bool Location::InMap()
{
    if (!GetValid()) { return false; }

    Map* map = RogueDataManager::Get()->ResolveByTypeIndex<Map>(z());
    return x() >= 0 && y() >= 0 && x() < map->m_size.x && y() < map->m_size.y;
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
    Map* map = RogueDataManager::Get()->ResolveByTypeIndex<Map>(z());
    ASSERT(map != nullptr);

    switch (direction)
    {
        case North:
            return std::make_pair(map->WrapLocation(*this, 0, 1), North);
        case NorthEast:
            return std::make_pair(map->WrapLocation(*this, 1, 1), North);
        case East:
            return std::make_pair(map->WrapLocation(*this, 1, 0), North);
        case SouthEast:
            return std::make_pair(map->WrapLocation(*this, 1, -1), North);
        case South:
            return std::make_pair(map->WrapLocation(*this, 0, -1), North);
        case SouthWest:
            return std::make_pair(map->WrapLocation(*this, -1, -1), North);
        case West:
            return std::make_pair(map->WrapLocation(*this, -1, 0), North);
        case NorthWest:
            return std::make_pair(map->WrapLocation(*this, -1, 1), North);
        default:
            return std::make_pair(*this, North);
    }
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
    if (GetValid() && RogueDataManager::Get()->CanResolve<Map>(z()) && InMap())
    {
        linkedTile = &RogueDataManager::Get()->ResolveByTypeIndex<Map>(z())->GetTile(x(), y());
    }
    else
    {
        linkedTile = nullptr;
    }
}
#endif

namespace RogueSaveManager
{
    void Serialize(Vec2& value)
    {
        AddOffset();
        Write("x", value.x);
        Write("y", value.y);
        RemoveOffset();
    }

    void Deserialize(Vec2& value)
    {
        AddOffset();
        Read("x", value.x);
        Read("y", value.y);
        RemoveOffset();
    }

    void Serialize(Vec3& value)
    {
        AddOffset();
        Write("x", value.x);
        Write("y", value.y);
        Write("z", value.z);
        RemoveOffset();
    }

    void Deserialize(Vec3& value)
    {
        AddOffset();
        Read("x", value.x);
        Read("y", value.y);
        Read("z", value.z);
        RemoveOffset();
    }

    void Serialize(Location& value)
    {
        AddOffset();
#ifdef _DEBUG
        Write("Valid", value.GetValid());
        if (value.GetValid())
        {
            Write("Vector", value.GetVector());
        }
#else
     Write("Data", value.GetData());
#endif
        RemoveOffset();
    }

    void Deserialize(Location& value)
    {
        AddOffset();
#ifdef _DEBUG
        value.SetValid(Read<bool>("Valid"));
        if (value.GetValid())
        {
            value.SetVector(Read<Vec3>("Vector"));
        }
#else
        value.SetData(Read<int>("Data"));
#endif
        RemoveOffset();
    }

    void Serialize(Color& value)
    {
        AddOffset();
#ifdef _DEBUG
        Write("r", (uint) value.r);
        Write("g", (uint) value.g);
        Write("b", (uint) value.b);
        Write("a", (uint) value.a);
#else
        Write("color_t", value.color);
#endif
        RemoveOffset();
    }

    void Deserialize(Color& value)
    {
        AddOffset();
#ifdef _DEBUG
        value.r = Read<uint>("r");
        value.g = Read<uint>("g");
        value.b = Read<uint>("b");
        value.a = Read<uint>("a");
#else
        Read("color_t", value.color);
#endif
        RemoveOffset();
    }
}