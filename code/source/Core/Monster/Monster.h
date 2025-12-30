#pragma once
#include "Core/CoreDataTypes.h"
#include "Data/Resources.h"
#include "Data/Serialization/Serialization.h"
#include "Core/Collections/FixedArrary.h"
#include "LOS/LOS.h"

class BodyDriver;
class MovementDriver;
class Monster;

namespace Serialization
{
	template<typename Stream>
	void Serialize(Stream& stream, const Monster& value);
	template<typename Stream>
	void Deserialize(Stream& stream, Monster& value);
}

struct BodyTileDefinitions
{
	char sprite;
	Color color;
};

struct BodyTile
{
	Location location;
};

class MonsterDefinition
{
public:
	MonsterDefinition() {}
	MonsterDefinition(BodyDriver* bodyDriver, MovementDriver* movementDriver, vector<BodyTileDefinitions> definitions) : m_bodyDriver(bodyDriver), m_movementDrivers({ movementDriver }), m_bodyTiles(definitions) {}

	BodyDriver* m_bodyDriver;
	std::vector<MovementDriver*> m_movementDrivers;
	vector<BodyTileDefinitions> m_bodyTiles;
};

class Monster
{
public:
	Monster() {}
	Monster(TResourcePointer<MonsterDefinition> definition);

	Location GetLocation();
	void SetLocation(Location newLocation);

	View& GetView() { return m_view; }
	Direction GetRotation() { return m_rotation; }
	void SetRotation(Direction rotation);
	
	bool Move(Direction direction);
	MovementDriver* GetDriverForMovement(Direction direction);

	bool CanMove(Direction direction);
	bool CouldAnyMovementStandOn(Location location);

	TResourcePointer<MonsterDefinition> GetDefinition() { return m_definition; }

private:
	TResourcePointer<MonsterDefinition> m_definition;

	Location m_location;
	View m_view;
	Direction m_rotation;

	vector<BodyTile> m_bodyTiles;

	template<typename Stream>
	friend void Serialization::Serialize(Stream& stream, const Monster& value);
	template<typename Stream>
	friend void Serialization::Deserialize(Stream& stream, Monster& value);
	friend class BodyDriver;
};

struct Movement
{
	Location m_location;
	float m_cost;

	static Movement FromLocation(Location location);
};

using MovementArray = FixedArray<Movement, 20>;

/* Additive movement modifiers
 * 
 * Gives monsters access to different forms of movement that they can choose from. (Swimming, flying, digging, walking, etc.)
 * 
 * Need some sort of hash signature for the maps they generate, for caching - most monsters share most of these, don't want to recalculate every frame if we can!
 */
class MovementDriver
{
public:
	virtual	bool CanStandOn(Location location) = 0;
	virtual void GetConnectedMovements(Location location, MovementArray& outMovements) = 0;
	virtual void OnMovementTaken(Monster& monster);
	virtual void OnMovedOnTile(Monster& monster, Location location, float& cost);
	void FillValidMovements(Location location, MovementArray& outMovements);
};

/* Semi-effect type of class

   Drives base-level body movement in monsters

   Pairs with movement drivers to determine what kinds of moves are allowed

   Acts as a movement operator of sorts - the final call on what kinds of shapes or positions are allowed

   E.G., you could move into this tile, but your body won't fit because it has a unique shape or your tail would be in the way
*/
class BodyDriver : public MovementDriver
{
public:
	BodyDriver() {}

	//MovementDriver* FindBestDriverForMovement(Monster& monster, StackArray<Location>& path, Direction direction);
	//bool CanMovementStandOn(Monster& monster, MovementDriver* driver, StackArray<Location>& path, Location location) = 0;

	virtual void InitMonster(Monster& monster, TResourcePointer<MonsterDefinition> definition) = 0;

	//For reorganizing the body after a movment!
	virtual void OnMovementFinished(THandle<Monster> monster) = 0;
};

namespace Serialization
{
	template<typename Stream>
	void Serialize(Stream& stream, const Monster& value)
	{
		//Write(stream, "Definition", value.m_definition);
	}

	template<typename Stream>
	void Deserialize(Stream& stream, Monster& value)
	{
		PRINT_ERR("Whoops, can't deserialize monsters yet.");
		HALT();
		//Read(stream, "Definition", value.m_definition);
	}
}