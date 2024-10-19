#pragma once
#include "Core/CoreDataTypes.h"
#include <vector>

class Tile;
class Map;
class View;
class Location;

template<typename T>
class THandle;

class Window;

using namespace std;

class TileMemory
{
public:
	TileMemory() {}
	TileMemory(THandle<Map> map);

	void Update(View& los);
	void Wipe();
	void Render(Window* window);
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
	bool ValidTile(int x, int y);
	Tile& GetTileByLocal(int x, int y);
	int IndexIntoTilemap(short x, short y);

	vector<Tile> m_tiles;

	Vec2 m_size;
	Vec2 m_localPosition;
	int m_z = -1;
	bool m_wrap = true;
};

namespace RogueSaveManager
{
	void Serialize(TileMemory& value);
	void Deserialize(TileMemory& value);
}