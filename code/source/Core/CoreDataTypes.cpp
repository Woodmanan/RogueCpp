#include "CoreDataTypes.h"
#include "Data/RogueDataManager.h"
#include "Data/SaveManager.h"
#include "Map/Map.h"
#include "Debug/Profiling.h"
#include "Game/ThreadManagers.h"

Vec4 Vec4::WrapPosition(Vec4 inPosition)
{
    return Vec4(
        ModulusNegative<int>(inPosition.x, LOCATION_MAX_X),
        ModulusNegative<int>(inPosition.y, LOCATION_MAX_Y),
        ModulusNegative<int>(inPosition.z, LOCATION_MAX_Z),
        ModulusNegative<int>(inPosition.w, LOCATION_MAX_W)
    );
}

Vec4 Vec4::WrapChunk(Vec4 inChunk)
{
    return Vec4(
        ModulusNegative<int>(inChunk.x, CHUNK_MAX_X),
        ModulusNegative<int>(inChunk.y, CHUNK_MAX_Y),
        ModulusNegative<int>(inChunk.z, CHUNK_MAX_Z),
        ModulusNegative<int>(inChunk.w, CHUNK_MAX_W)
    );
}

Location::Location()
{
	m_vec = Vec4(0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF);

	ASSERT(!GetValid());

#ifdef LINK_TILE
    RefreshLinkedTile();
#endif
}

Location::Location(uint x, uint y, uint z, uint w)
{	
	SetVector(Vec4(x,y,z,w));

#ifdef LINK_TILE
    RefreshLinkedTile();
#endif
}

bool Location::GetValid() const
{
    return !(m_vec.x & invalidMask);
}

void Location::SetValid(bool valid)
{
    if (valid)
    {
        m_vec.x &= (~invalidMask);
    }
    else
    {
        m_vec.x |= invalidMask;
    }
}

uint Location::x() const
{
    ASSERT(GetValid());
    return (uint) m_vec.x;
}

uint Location::y() const
{
    ASSERT(GetValid());
    return (uint) m_vec.y;
}

uint Location::z() const
{
    ASSERT(GetValid());
    return (uint) m_vec.z;
}

uint Location::w() const
{
    ASSERT(GetValid());
    return (uint) m_vec.w;
}

Vec4 Location::GetVector() const
{
    ASSERT(GetValid());
	return m_vec;
}

void Location::SetVector(Vec4 vector)
{
	ASSERT(vector.x >= 0 && vector.x < LOCATION_MAX_X);
    ASSERT(vector.y >= 0 && vector.y < LOCATION_MAX_Y);
    ASSERT(vector.z >= 0 && vector.z < LOCATION_MAX_Z);
    ASSERT(vector.w >= 0 && vector.w < LOCATION_MAX_W);

	m_vec = vector;
	ASSERT(GetValid());
}

Vec2 Location::AsVec2()
{
    ASSERT(GetValid());
	return (Vec2) GetVector();
}

Vec4 Location::GetChunkPosition()
{
    ASSERT(GetValid());
	return m_vec / Vec4(CHUNK_SIZE_X, CHUNK_SIZE_Y, CHUNK_SIZE_Z, CHUNK_SIZE_W);
}

Vec4 Location::GetChunkLocalPosition()
{
    ASSERT(GetValid());
	return m_vec % Vec4(CHUNK_SIZE_X, CHUNK_SIZE_Y, CHUNK_SIZE_Z, CHUNK_SIZE_W);
}

Tile& Location::GetTile() const
{
	ASSERT(GetValid());
    return GetDataManager()->ResolveByTypeIndex<ChunkMap>(0)->GetTile(*this);
}

Tile* Location::operator ->() const
{
	ASSERT(GetValid());
    return &GetDataManager()->ResolveByTypeIndex<ChunkMap>(0)->GetTile(*this);
}

Location Location::GetNeighbor(Direction direction)
{
    ASSERT(GetValid());
    if (HasNeighbors())
    {
		THandle<TileNeighbors> neighbors = GetNeighbors();

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
        Vec4 offset = VectorFromDirection(direction);
        return Location(Vec4::WrapPosition(m_vec + offset));
    }
}

void Location::SetNeighbor(Direction direction, Location location, Direction rotation)
{
    ASSERT(GetValid());
	THandle<TileNeighbors> neighbors = GetOrCreateNeighbors();
	switch (direction)
	{
		case North:
			neighbors->N = location;
			neighbors->N_Direction = rotation;
			break;
		case NorthEast:
			neighbors->NE = location;
			neighbors->NE_Direction = rotation;
			break;
		case East:
			neighbors->E = location;
			neighbors->E_Direction = rotation;
			break;
		case SouthEast:
			neighbors->SE = location;
			neighbors->SE_Direction = rotation;
			break;
		case South:
			neighbors->S = location;
			neighbors->S_Direction = rotation;
			break;
		case SouthWest:
			neighbors->SW = location;
			neighbors->SW_Direction = rotation;
			break;
		case West:
			neighbors->W = location;
			neighbors->W_Direction = rotation;
			break;
		case NorthWest:
			neighbors->NW = location;
			neighbors->NW_Direction = rotation;
			break;
	}
}

std::pair<Location, Direction> Location::Traverse(Vec2 offset, Direction rotation)
{
    ASSERT(GetValid());
    return Traverse(offset.x, offset.y, rotation);
}

std::pair<Location, Direction> Location::Traverse(short xOffset, short yOffset, Direction rotation)
{
    ASSERT(GetValid());
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
    ASSERT(GetValid());
    Direction finalDirection = Rotate(direction, rotation);

    if (HasNeighbors())
    {
        return _Traverse_Neighbors(finalDirection);
    }
    else
    {
        return _Traverse_No_Neighbor(finalDirection);
    }
}

std::pair<Location, Direction> Location::_Traverse_No_Neighbor(Direction direction)
{
    ROGUE_PROFILE_SECTION("LOS::_Traverse_No_Neighbor");
    ASSERT(GetValid());
    ASSERT((!GetTile().m_stats.IsValid() || !GetTile().m_stats->m_neighbors.IsValid()));
    return std::make_pair(GetNeighbor(direction), North);
}

std::pair<Location, Direction> Location::_Traverse_Neighbors(Direction direction)
{
    ROGUE_PROFILE_SECTION("LOS::_Traverse_Neighbors");
    ASSERT(GetValid());
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

bool Location::UsingInstanceData() const
{
	ASSERT(GetValid());
    return GetTile().UsingInstanceData();
}

void Location::CreateInstanceData() const
{
	ASSERT(GetValid());
	ASSERT(!UsingInstanceData());
	GetTile().CreateInstanceData();
}

THandle<TileStats> Location::GetOrCreateInstanceData() const
{
	if (!UsingInstanceData())
	{
		CreateInstanceData();
	}

	return GetInstanceData();
}

THandle<TileStats> Location::GetInstanceData() const
{
	ASSERT(UsingInstanceData());

	return GetTile().m_stats;
}

bool Location::HasNeighbors() const
{
	return UsingInstanceData() && GetInstanceData()->m_neighbors.IsValid();
}

void Location::CreateDefaultNeighbors() const
{
	ASSERT(GetValid());
	if (HasNeighbors())
	{
		return;
	}

	THandle<TileStats> stats = GetOrCreateInstanceData();
	THandle<TileNeighbors> neighbors = GetDataManager()->Allocate<TileNeighbors>();
	stats->m_neighbors = neighbors;

	neighbors->N  = Location(Vec4::WrapPosition(m_vec + Vec4(0, 1)));
	neighbors->NE = Location(Vec4::WrapPosition(m_vec + Vec4(1, 1)));
	neighbors->E  = Location(Vec4::WrapPosition(m_vec + Vec4(1, 0)));
	neighbors->SE = Location(Vec4::WrapPosition(m_vec + Vec4(1, -1)));
	neighbors->S  = Location(Vec4::WrapPosition(m_vec + Vec4(0, -1)));
	neighbors->SW = Location(Vec4::WrapPosition(m_vec + Vec4(-1, -1)));
	neighbors->W  = Location(Vec4::WrapPosition(m_vec + Vec4(-1, 0)));
	neighbors->NW = Location(Vec4::WrapPosition(m_vec + Vec4(-1, 1)));
	
	neighbors->N_Direction = North;
	neighbors->NE_Direction = North;
	neighbors->E_Direction = North;
	neighbors->SE_Direction = North;
	neighbors->S_Direction = North;
	neighbors->SW_Direction = North;
	neighbors->W_Direction = North;
	neighbors->NW_Direction = North;

	ASSERT(HasNeighbors());
}

THandle<TileStats> Location::GetOrCreateNeighbors() const
{
	ASSERT(GetValid());
	if (!HasNeighbors())
	{
		CreateDefaultNeighbors();
	}

	return GetNeighbors();
}

THandle<TileNeighbors> Location::GetNeighbors() const
{
	ASSERT(GetValid());
	ASSERT(HasNeighbors());

	return GetInstanceData()->m_neighbors;
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
