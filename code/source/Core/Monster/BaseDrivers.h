#include "Monster.h"

class WalkingDriver : MovementDriver
{
	virtual	bool CanStandOn(Location location) override;
	virtual void GetConnectedPositions(Location location, StackArray<Location>& locations) override;
};