#include "BaseDrivers.h"
#include "Map/Map.h"

bool WalkingDriver::CanStandOn(Location location)
{
	return location.GetValid() && !location->m_wall;
}

void WalkingDriver::GetConnectedMovements(Location location, MovementArray& outMovements)
{
	outMovements.push_back(Movement::FromLocation(location.GetNeighbor(Direction::North)));
	outMovements.push_back(Movement::FromLocation(location.GetNeighbor(Direction::East)));
	outMovements.push_back(Movement::FromLocation(location.GetNeighbor(Direction::South)));
	outMovements.push_back(Movement::FromLocation(location.GetNeighbor(Direction::West)));
	outMovements.push_back(Movement::FromLocation(location.GetNeighbor(Direction::NorthEast)));
	outMovements.push_back(Movement::FromLocation(location.GetNeighbor(Direction::SouthEast)));
	outMovements.push_back(Movement::FromLocation(location.GetNeighbor(Direction::SouthWest)));
	outMovements.push_back(Movement::FromLocation(location.GetNeighbor(Direction::NorthWest)));
}

bool MiningDriver::CanStandOn(Location location)
{
	return location.GetValid();
}

void MiningDriver::GetConnectedMovements(Location location, MovementArray& outMovements)
{
	WalkingDriver::GetConnectedMovements(location, outMovements);
	for (Movement& movement : outMovements)
	{
		if (movement.m_location->m_wall)
		{
			movement.m_cost *= 3;
		}
	}
}

void MiningDriver::OnMovedOnTile(Monster& monster, Location location, float& cost)
{
	if (location->m_wall)
	{
		location->BreakWall();
	}
}

bool RectangularBodyDriver::CanMonsterStandOn(Monster& monster, Location location)
{
	//TODO: Assert connectedness

	//TODO: Determine if we allow any movement to validate a position, or if the position needs to be supported by only 1

	Location base = location;
	Location xMovement = base;
	for (int x = 0; x < m_dimensions.x; x++)
	{
		Location yMovement = xMovement;
		for (int y = 0; y < m_dimensions.y; y++)
		{
			//Check location
			if (!monster.CouldAnyMovementStandOn(yMovement))
			{
				return false;
			}

			//Move down
			yMovement = yMovement.Traverse(Direction::South).first;
		}
		xMovement = xMovement.Traverse(Direction::East).first;
	}

	return true;
}

void RectangularBodyDriver::InitMonster(Monster& monster, TResourcePointer<MonsterDefinition> definition)
{

}

//For reorganizing the body after a movment!
void RectangularBodyDriver::OnMovementFinished(THandle<Monster> monster)
{

}