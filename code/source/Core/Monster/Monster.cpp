#include "Monster.h"
#include "Map/Map.h"

Monster::Monster(TResourcePointer<MonsterDefinition> definition) : m_definition(definition)
{
	ASSERT(definition.IsValid() && definition.IsReady());
	//ASSERT(definition->m_bodyDriver != nullptr);
	ASSERT(!definition->m_movementDrivers.empty());

	//m_bodyTiles.resize(definition->m_bodyTiles.size());

	//definition->m_bodyDriver->InitMonster(*this, definition);
}

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

void Monster::GetAllowedMovements(Location location, MovementArray& outMovements)
{
	for (MovementDriver* driver : m_definition->m_movementDrivers)
	{
		MovementArray next;
		driver->FillValidMovements(location, next);

		for (Movement& move : next)
		{
			Movement* match = outMovements.find([&move](Movement& other)
				{
					return other.m_location == move.m_location;
				});

			if (match)
			{
				match->m_cost = std::min(move.m_cost, match->m_cost);
			}
			else if (m_definition->m_bodyDriver->CanMonsterStandOn(*this, move.m_location))
			{
				outMovements.push_back(move);
			}
		}
	}
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
	driver->OnMovedOnTile(*this, move.first, cost); //TODO: Real costing!

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
			//TODO: Make this a ranking search
			MovementArray validMoves;
			driver->GetConnectedMovements(m_location, validMoves);

			for (const Movement& movement : validMoves)
			{
				if (movement.m_location == destination)
				{
					return driver;
				}
			}
		}
	}

	return nullptr;
}

bool Monster::CanMove(Direction direction)
{
	return (GetDriverForMovement(direction) != nullptr);
}

bool Monster::CouldAnyMovementStandOn(Location location)
{
	for (MovementDriver* driver : m_definition->m_movementDrivers)
	{
		if (driver->CanStandOn(location))
		{
			return true;
		}
	}

	return false;
}

Movement Movement::FromLocation(Location location)
{
	return { location, location->m_movementCost };
}

void MovementDriver::OnMovementTaken(Monster& monster)
{

}

void MovementDriver::OnMovedOnTile(Monster& monster, Location location, float& cost)
{

}

void MovementDriver::FillValidMovements(Location location, MovementArray& outMovements)
{
	MovementArray allMovements;
	GetConnectedMovements(location, allMovements);

	for (Movement& move : allMovements)
	{
		if (CanStandOn(move.m_location))
		{
			outMovements.push_back(move);
		}
	}
}