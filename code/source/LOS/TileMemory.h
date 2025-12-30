#pragma once
#include "Core/CoreDataTypes.h"
#include "Core/Math/Math.h"
#include "Map/Map.h"
#include "Core/Materials/Materials.h"
#include "Data/Serialization/BitStream.h"
#include "Data/Serialization/Serialization.h"
#include <vector>

class Tile;
class Map;
class View;
class Location;

template<typename T>
class THandle;

class Window;

using namespace std;

enum class ETemperature
{
	Freezing,
	Cold,
	Average,
	Hot,
	Burning
};

struct DataTile
{
	THandle<BackingTile> m_backingTile;
	ETemperature m_temperature = ETemperature::Average;
	char m_renderChar;
	Color m_color;

	static DataTile FromTile(const Tile& tile);
	bool operator==(const Tile& other);
};

#define INTERLEAVE_TILES
static constexpr uint INTERLEAVE_VALUE = 8;
static constexpr uint MEMORY_RADIUS = 127;
static constexpr uint MEMORY_SIZE = RoundUpNearestMultiple((2 * MEMORY_RADIUS) + 1, INTERLEAVE_VALUE);
static constexpr uint MEMORY_BUFFER_SIZE = MEMORY_SIZE * MEMORY_SIZE;

class TileMemory
{
public:
	TileMemory();

	void Update(View& los);
	void Wipe();
	void Move(Vec2 offset);

	void SetLocalPosition(Location location);
	void SetTileByLocal(int x, int y, Location location);
	void SetTileByLocal(int x, int y, const Tile& tile);
	void SetTileByLocal(int x, int y, const DataTile& tile);
	bool ValidTile(int x, int y);
	DataTile& GetTileByLocal(int x, int y);
	int IndexIntoTilemap(short x, short y);

	vector<DataTile> m_tiles;
	vector<Location> m_realLocations;
	Vec2 m_localPosition;

private:
	void Debug_CheckIndexingDuplicates();
	void WipeLocalRect(Vec2 position, Vec2 size);

	static Vec2 WrapVector(Vec2 vector);
};

namespace Serialization
{
	template<typename Stream>
	void SerializeObject(Stream& stream, const ETemperature& value)
	{
		stream.WriteEnum(value);
	}

	template<typename Stream>
	void DeserializeObject(Stream& stream, ETemperature& value)
	{
		stream.ReadEnum(value);
	}

	template<typename Stream>
	void Serialize(Stream& stream, const DataTile& value)
	{
		Write(stream, "Backing Tile", value.m_backingTile);
		Write(stream, "Temperature", value.m_temperature);
		Write(stream, "Render Char", value.m_renderChar);
		Write(stream, "Visible Color", value.m_color);
	}

	template<typename Stream>
	void Deserialize(Stream& stream, DataTile& value)
	{
		Read(stream, "Backing Tile", value.m_backingTile);
		Read(stream, "Temperature", value.m_temperature);
		Read(stream, "Render Char", value.m_renderChar);
		Read(stream, "Visible Color", value.m_color);
	}

	template<typename Stream>
	void Serialize(Stream& stream, const TileMemory& value)
	{
		Write(stream, "Local Position", value.m_localPosition);
		Write(stream, "Tile Memory", value.m_tiles);
	}

	template<typename Stream>
	void Deserialize(Stream& stream, TileMemory& value)
	{
		Read(stream, "Local Position", value.m_localPosition);
		Read(stream, "Tile Memory", value.m_tiles);
	}
}