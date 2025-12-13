#include "Monster.h"
#include "Map/Map.h"

Location Monster::GetLocation()
{
	return m_location;
}

void Monster::SetLocation(Location newLocation)
{
	m_location = newLocation;
}

void Monster::SetRotation(Direction rotation)
{
	m_rotation = rotation;
}

bool Monster::Move(Direction direction)
{
	MovementDriver* driver = GetDriverForMovement(direction);
	if (driver == nullptr)
	{
		return false;
	}

	auto move = m_location.Traverse(direction, m_rotation);
	SetLocation(move.first);
	SetRotation(Rotate(m_rotation, move.second));

	float cost = 1.0f;
	driver->OnMovedOnTile(move.first, *this, cost); //TODO: Real costing!

	return true;
}

MovementDriver* Monster::GetDriverForMovement(Direction direction)
{
	ASSERT(m_definition.IsValid() && m_definition.IsReady());
	ASSERT(!m_definition->m_movementDrivers.empty());

	Location destination = m_location.Traverse(direction, m_rotation).first;
	for (MovementDriver* driver : m_definition->m_movementDrivers)
	{
		if (!driver->CanStandOn(destination))
		{
			continue;
		}
		else
		{
			STACKARRAY(Location, validMoves, 30);
			driver->FillValidPositions(m_location, validMoves);

			if (validMoves.contains(destination))
			{
				return driver;
			}
		}
	}

	return nullptr;
}

bool Monster::CanMove(Direction direction)
{
	return (GetDriverForMovement(direction) != nullptr);
}

void MovementDriver::OnMovementTaken(Monster& monster)
{

}

void MovementDriver::OnMovedOnTile(Location location, Monster& monster, float& cost)
{

}

void MovementDriver::FillValidPositions(Location location, StackArray<Location>& locations)
{
	STACKARRAY(Location, allPositions, 20);
	GetConnectedPositions(location, allPositions);

	for (Location& loc : allPositions)
	{
		AddIfValid(loc, locations);
	}
}

void MovementDriver::AddIfValid(Location location, StackArray<Location>& locations)
{
	if (CanStandOn(location))
	{
		locations.push_back(location);
	}
}