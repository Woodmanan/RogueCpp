#include "Core/CoreDataTypes.h"
#include "Data/Resources.h"
#include "Data/Serialization/Serialization.h"
#include "Core/Collections/StackArray.h"

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

struct TileBody
{
	Vec3 position;
	char sprite;
	Color color;
};

class MonsterDefinition
{
	BodyDriver* m_bodyDriver;
	std::vector<MovementDriver*> m_movementDrivers;
};

class Monster
{
public:
	Monster() {}

private:
	TResourcePointer<MonsterDefinition> m_definition;

	template<typename Stream>
	friend void Serialization::Serialize(Stream& stream, const Monster& value);
	template<typename Stream>
	friend void Serialization::Deserialize(Stream& stream, Monster& value);
	friend class BodyDriver;
};

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
	virtual void GetConnectedPositions(Location location, StackArray<Location>& locations) = 0;
	void FillValidPositions(Location location, StackArray<Location>&locations);
	void AddIfValid(Location location, StackArray<Location>& locations);
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

	//For reorganizing the body after a movment!
	virtual void OnMovementFinished(THandle<Monster> monster) = 0;
};

namespace Serialization
{
	template<typename Stream>
	void Serialize(Stream& stream, const Monster& value)
	{
		Write(stream, "Definition", value.m_definition);
	}

	template<typename Stream>
	void Deserialize(Stream& stream, Monster& value)
	{
		Read(stream, "Definition", value.m_definition);
	}
}