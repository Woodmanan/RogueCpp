#pragma once
#include "Core/CoreDataTypes.h"
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

class TileMemory
{
public:
	TileMemory() {}
	TileMemory(THandle<Map> map);

	void Update(View& los);
	void Wipe();
	void Move(Vec2 offset)
	{
		m_localPosition += offset; 
		if (m_wrap)
		{
			m_localPosition.x = (m_localPosition.x + m_size.x) % m_size.x;
			m_localPosition.y = (m_localPosition.y + m_size.y) % m_size.y;
		}
	}

	void SetLocalPosition(Location location);
	void SetTileByLocal(int x, int y, Location location);
	void SetTileByLocal(int x, int y, const Tile& tile);
	void SetTileByLocal(int x, int y, const DataTile& tile);
	bool ValidTile(int x, int y);
	DataTile& GetTileByLocal(int x, int y);
	int IndexIntoTilemap(short x, short y);

	vector<DataTile> m_tiles;

	Vec2 m_size;
	Vec2 m_localPosition;
	int m_z = -1;
	bool m_wrap = true;
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
		Write(stream, "Size", value.m_size);
		Write(stream, "Local Position", value.m_localPosition);
		Write(stream, "Z Level", value.m_z);
		Write(stream, "Wraps", value.m_wrap);
		Write(stream, "Tile Memory", value.m_tiles);
	}

	template<typename Stream>
	void Deserialize(Stream& stream, TileMemory& value)
	{
		Read(stream, "Size", value.m_size);
		Read(stream, "Local Position", value.m_localPosition);
		Read(stream, "Z Level", value.m_z);
		Read(stream, "Wraps", value.m_wrap);
		Read(stream, "Tile Memory", value.m_tiles);
	}
}