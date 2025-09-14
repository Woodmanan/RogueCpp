#include "TileMemory.h"
#include "Map/Map.h"
#include "Data/SaveManager.h"
#include "Data/RogueDataManager.h"
#include "Render/Windows/Window.h"
#include "Render/Terminal.h"
#include "LOS.h"
#include <algorithm>

#define INTERLEAVE_TILES
static const unsigned int masks[] = { 0x00, 0x01, 0x03, 0x07, 0x0F, 0x1F, 0x3F, 0x7F, 0xFF };
static const unsigned int interleaveBits = 4;

TileMemory::TileMemory(THandle<Map> map)
{
	ASSERT(map.IsValid());
	m_z = map->z;
	m_size = map->m_size;
	m_tiles.resize(m_size.x * m_size.y, Tile());
}

void TileMemory::Update(View& los)
{
	for (int x = -los.GetRadius(); x <= los.GetRadius(); x++)
	{
		for (int y = -los.GetRadius(); y <= los.GetRadius(); y++)
		{
			if (los.GetVisibilityLocal(x, y))
			{
				SetTileByLocal(x, y, los.GetLocationLocal(x, y));
			}
		}
	}
}

void TileMemory::Wipe()
{
	std::fill(m_tiles.begin(), m_tiles.end(), Tile());
}

void TileMemory::SetLocalPosition(Location location)
{
	m_localPosition = Vec2(location.x(), location.y());
}

void TileMemory::SetTileByLocal(int x, int y, Location location)
{
	ASSERT(location.GetValid());
	if (m_wrap)
	{
		x = ((x + m_size.x) + m_localPosition.x) % m_size.x;
		y = ((y + m_size.y) + m_localPosition.y) % m_size.y;
	}

	if (x < 0 || x >= m_size.x || y < 0 || y >= m_size.y) { return; }

	m_tiles[IndexIntoTilemap(x, y)] = location.GetTile();
}

void TileMemory::SetTileByLocal(int x, int y, Tile tile)
{
	if (m_wrap)
	{
		x = ((x + m_size.x) + m_localPosition.x) % m_size.x;
		y = ((y + m_size.y) + m_localPosition.y) % m_size.y;
	}

	if (x < 0 || x >= m_size.x || y < 0 || y >= m_size.y) { return; }

	m_tiles[IndexIntoTilemap(x, y)] = tile;
}

bool TileMemory::ValidTile(int x, int y)
{
	if (m_wrap)
	{
		x = ((x + m_size.x) + m_localPosition.x) % m_size.x;
		y = ((y + m_size.y) + m_localPosition.y) % m_size.y;
	}

	return (x >= 0 && x < m_size.x && y >= 0 && y < m_size.y);
}

Tile& TileMemory::GetTileByLocal(int x, int y)
{
	ASSERT(ValidTile(x, y));
	if (m_wrap)
	{
		x = ((x + m_size.x) + m_localPosition.x) % m_size.x;
		y = ((y + m_size.y) + m_localPosition.y) % m_size.y;
	}

	return m_tiles[IndexIntoTilemap(x, y)];
}

int TileMemory::IndexIntoTilemap(short x, short y)
{
#ifdef INTERLEAVE_TILES
	int bigX = x >> interleaveBits;
	int bigY = y >> interleaveBits;
	int bigBlockIndex = (bigX + (m_size.x >> (interleaveBits)) * bigY) << (interleaveBits * 2);

	int lilX = x & masks[interleaveBits];
	int lilY = y & masks[interleaveBits];
	return bigBlockIndex + lilX + (1 << interleaveBits) * lilY;
#else // INTERLEAVE
	return (location.x + (m_size.x * location.y));
#endif
}