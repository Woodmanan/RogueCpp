#include "Monster.h"

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