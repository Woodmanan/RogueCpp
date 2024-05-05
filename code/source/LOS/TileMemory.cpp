#include "TileMemory.h"
#include "../Map/Map.h"
#include "../Data/SaveManager.h"
#include "../Data/RogueDataManager.h"
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
				SetTileByLocal(x + m_localPosition.x, y + m_localPosition.y, los.GetLocationLocal(x, y));
			}
		}
	}
}

void TileMemory::Wipe()
{
	std::fill(m_tiles.begin(), m_tiles.end(), Tile());
}

void TileMemory::Render(Vec2 window)
{
	color_t black = color_from_argb(0xFF, 0x00, 0x00, 0x00);
	color_t empty = color_from_argb(0x00, 0x00, 0x00, 0x00);
	terminal_bkcolor(empty);

	for (int i = 0; i < window.x; i++)
	{
		for (int j = 0; j < window.y; j++)
		{
			int x = m_localPosition.x + i - 40;
			int y = m_localPosition.y - (j - 20);

			if (ValidTile(x, y))
			{
				Tile& tile = GetTileByLocal(x, y);

				if (tile.m_backingTile.IsValid())
				{
					terminal_color(Blend(tile.m_backingTile->m_foregroundColor, black, 0.5f));
					
					terminal_put(i, j, tile.m_backingTile->m_renderCharacter);
				}
			}
		}
	}
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
		x = (x + m_size.x) % m_size.x;
		y = (y + m_size.y) % m_size.y;
	}

	if (x < 0 || x >= m_size.x || y < 0 || y >= m_size.y) { return; }

	m_tiles[IndexIntoTilemap(x, y)] = location.GetTile();
}

bool TileMemory::ValidTile(int x, int y)
{
	if (m_wrap)
	{
		x = (x + m_size.x) % m_size.x;
		y = (y + m_size.y) % m_size.y;
	}

	return (x >= 0 && x < m_size.x && y >= 0 && y < m_size.y);
}

Tile& TileMemory::GetTileByLocal(int x, int y)
{
	ASSERT(ValidTile(x, y));
	if (m_wrap)
	{
		x = (x + m_size.x) % m_size.x;
		y = (y + m_size.y) % m_size.y;
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

namespace RogueSaveManager
{
	void Serialize(TileMemory& value)
	{
		AddOffset();
		Write("Size", value.m_size);
		Write("Local Position", value.m_localPosition);
		Write("Z Level", value.m_z);
		Write("Wraps", value.m_wrap);
		Write("Tile Memory", value.m_tiles);
		RemoveOffset();
	}

	void Deserialize(TileMemory& value)
	{
		AddOffset();
		Read("Size", value.m_size);
		Read("Local Position", value.m_localPosition);
		Read("Z Level", value.m_z);
		Read("Wraps", value.m_wrap);
		Read("Tile Memory", value.m_tiles);
		RemoveOffset();
	}
}