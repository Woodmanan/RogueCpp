#include "BaseDrivers.h"
#include "Map/Map.h"

bool WalkingDriver::CanStandOn(Location location)
{
	return location.GetValid() && !location->m_wall;
}

void WalkingDriver::GetConnectedPositions(Location location, StackArray<Location>& locations)
{
	locations.push_back(location.GetNeighbor(Direction::North));
	locations.push_back(location.GetNeighbor(Direction::NorthEast));
	locations.push_back(location.GetNeighbor(Direction::East));
	locations.push_back(location.GetNeighbor(Direction::SouthEast));
	locations.push_back(location.GetNeighbor(Direction::South));
	locations.push_back(location.GetNeighbor(Direction::SouthWest));
	locations.push_back(location.GetNeighbor(Direction::West));
	locations.push_back(location.GetNeighbor(Direction::NorthWest));
}