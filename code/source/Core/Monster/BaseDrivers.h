#include "Monster.h"

class WalkingDriver : public MovementDriver
{
	virtual	bool CanStandOn(Location location) override;
	virtual void GetConnectedPositions(Location location, StackArray<Location>& locations) override;
};

class CrushingDriver : public WalkingDriver
{
	virtual	bool CanStandOn(Location location) override;
	virtual void OnMovedOnTile(Location location, Monster& monster, float& cost) override;
};