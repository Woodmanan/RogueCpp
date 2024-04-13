#include "CoreDataTypes.h"
#include "../Data/SaveManager.h"
#include "../Map/Map.h"

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
    m_data = invalidMask;
}

Location::Location(ushort x, ushort y, ushort z)
{
    static_assert((xSize + ySize + zSize + 1) == 32, "Location will not fit into an int!");
    int intX = (int)x;
    int intY = (int)y;
    int intZ = (int)z;

    m_data = ((intX << (ySize + zSize)) & xMask) |
             ((intY << (zSize))         & yMask) |
             ( intZ                     & zMask);
}

bool Location::GetValid()
{
    return !(m_data & invalidMask);
}

void Location::SetValid(bool valid)
{
    if (valid)
    {
        m_data &= (~invalidMask);
    }
    else
    {
        m_data |= invalidMask;
    }
}

ushort Location::x()
{
    ASSERT(GetValid());
    return (ushort)((m_data & xMask) >> (ySize + zSize));
}

ushort Location::y()
{
    ASSERT(GetValid());
    return (ushort)((m_data & yMask) >> (zSize));
}

ushort Location::z()
{
    ASSERT(GetValid());
    return (ushort)(m_data & zMask);
}

Vec3 Location::GetVector()
{
    return Vec3(x(), y(), z());
}

void Location::SetVector(Vec3 vector)
{
    int intX = (int)vector.x;
    int intY = (int)vector.y;
    int intZ = (int)vector.z;

    m_data = ((intX << (ySize + zSize)) & xMask) |
             ((intY << (zSize))         & yMask) |
             ( intZ                     & zMask);
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

Location Location::Traverse(Vec2 offset)
{
    return Traverse(offset.x, offset.y);
}

Location Location::Traverse(short xOffset, short yOffset)
{
    ASSERT(xOffset >= -1 && xOffset <= 1);
    ASSERT(yOffset >= -1 && yOffset <= 1);

    if (xOffset == 0 && yOffset == 0)
    {
        return Location(x(), y(), z());
    }

    short index = (xOffset + 1) + ((yOffset + 1) * 3);

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

    return Traverse(direction);
}

Location Location::Traverse(Direction direction)
{
    if (!GetTile().m_stats.IsValid() || !GetTile().m_stats->m_neighbors.IsValid())
    {
        return _Traverse_No_Neighbor(direction);
    }
    else
    {
        return _Traverse_Neighbors(direction);
    }
}

Location Location::_Traverse_No_Neighbor(Direction direction)
{
    ASSERT((!GetTile().m_stats.IsValid() || !GetTile().m_stats->m_neighbors.IsValid()));
    switch (direction)
    {
        case North:
            return Location(x() + 0, y() + 1, z());
        case NorthEast:
            return Location(x() + 1, y() + 1, z());
        case East:
            return Location(x() + 1, y() + 0, z());
        case SouthEast:
            return Location(x() + 1, y() - 1, z());
        case South:
            return Location(x() + 0, y() - 1, z());
        case SouthWest:
            return Location(x() - 1, y() - 1, z());
        case West:
            return Location(x() - 1, y() + 0, z());
        case NorthWest:
            return Location(x() - 1, y() + 1, z());
        default:
            return Location(x(), y(), z());
    }
}

Location Location::_Traverse_Neighbors(Direction direction)
{
    ASSERT(GetTile().m_stats.IsValid());
    THandle<TileNeighbors> neighbors = GetTile().m_stats->m_neighbors;
    ASSERT(neighbors.IsValid());
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
        if (debug)
        {
            Write("Valid", value.GetValid());
            if (value.GetValid())
            {
                Write("Vector", value.GetVector());
            }
        }
        else
        {
            Write("Data", value.GetData());
        }
        RemoveOffset();
    }

    void Deserialize(Location& value)
    {
        AddOffset();
        if (debug)
        {
            value.SetValid(Read<bool>("Valid"));
            if (value.GetValid())
            {
                value.SetVector(Read<Vec3>("Vector"));
            }
        }
        else
        {
            value.SetData(Read<int>("Data"));
        }
        RemoveOffset();
    }
}