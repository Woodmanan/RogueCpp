#include "Monster.h"

class WalkingDriver : public MovementDriver
{
protected:
	virtual	bool CanStandOn(Location location) override;
	virtual void GetConnectedMovements(Location location, MovementArray& outMovements) override;
};

class MiningDriver : public WalkingDriver
{
	virtual	bool CanStandOn(Location location) override;
	virtual void GetConnectedMovements(Location location, MovementArray& outMovements) override;
	virtual void OnMovedOnTile(Monster& monster, Location location, float& cost) override;
};

class RectangularBodyDriver : public BodyDriver
{
public:
	RectangularBodyDriver(Vec2 dimensions) : m_dimensions(dimensions) {}

	virtual bool CanMonsterStandOn(Monster& monster, Location location) override;
	virtual void InitMonster(Monster& monster, TResourcePointer<MonsterDefinition> definition) override;

	//For reorganizing the body after a movment!
	virtual void OnMovementFinished(THandle<Monster> monster) override;

private:
	Vec2 m_dimensions;
};