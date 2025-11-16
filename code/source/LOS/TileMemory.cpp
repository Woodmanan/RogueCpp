#include "TileMemory.h"
#include "Map/Map.h"
#include "Data/SaveManager.h"
#include "Data/RogueDataManager.h"
#include "Render/Windows/Window.h"
#include "Render/Terminal.h"
#include "LOS.h"
#include "Game/Game.h"
#include <algorithm>
#include <tuple>

ETemperature TemperatureFromHeat(float heat)
{
	if (heat <= 0.0f)
	{
		return ETemperature::Freezing;
	}
	else if (heat <= 20.0f)
	{
		return ETemperature::Cold;
	}
	else if (heat <= 40.0f)
	{
		return ETemperature::Average;
	}
	else if (heat <= 60)
	{
		return ETemperature::Hot;
	}
	else
	{
		return ETemperature::Burning;
	}
}

DataTile DataTile::FromTile(const Tile& tile)
{
	DataTile data;
	data.m_backingTile = tile.m_backingTile;
	data.m_temperature = TemperatureFromHeat(tile.m_heat);

	pair<int, bool> materialInfo = tile.GetVisibleMaterial();
	const MaterialDefinition& material = Game::materialManager->GetMaterialByID(materialInfo.first);
	if (materialInfo.second)
	{
		data.m_renderChar = material.wallChar;
	}
	else
	{
		data.m_renderChar = material.floorChar;
	}
	data.m_color = material.color;
	return data;
}

bool DataTile::operator==(const Tile& other)
{
	pair<int, bool> materialInfo = other.GetVisibleMaterial();
	const MaterialDefinition& material = Game::materialManager->GetMaterialByID(materialInfo.first);
	char renderChar = material.floorChar;
	if (materialInfo.second)
	{
		renderChar = material.wallChar;
	}
	const Color& color = material.color;
	return (m_backingTile == other.m_backingTile) && (m_temperature == TemperatureFromHeat(other.m_heat) && (m_renderChar == renderChar) && (m_color == color));
}

TileMemory::TileMemory()
{
	m_tiles.resize(MEMORY_BUFFER_SIZE, DataTile());

#ifdef DEBUG
	Debug_CheckIndexingDuplicates();
#endif
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
	std::fill(m_tiles.begin(), m_tiles.end(), DataTile());
}

void TileMemory::Move(Vec2 offset)
{
	ROGUE_PROFILE_SECTION("TileMemory::Move");
	//Pre move - need to wipe out the edges of this map!
	WipeLocalRect(Vec2(0, -MEMORY_RADIUS), Vec2(MEMORY_SIZE, 1));
	WipeLocalRect(Vec2(0, MEMORY_RADIUS), Vec2(MEMORY_SIZE, 1));
	WipeLocalRect(Vec2(-MEMORY_RADIUS, 0), Vec2(1, MEMORY_SIZE));
	WipeLocalRect(Vec2(MEMORY_RADIUS, 0), Vec2(1, MEMORY_SIZE));

	m_localPosition += offset;
}

void TileMemory::SetLocalPosition(Location location)
{
	m_localPosition = Vec2(location.x(), location.y());
}

void TileMemory::SetTileByLocal(int x, int y, Location location)
{
	ASSERT(location.GetValid());
	x = ModulusNegative<int>(x + m_localPosition.x, MEMORY_SIZE);
	y = ModulusNegative<int>(y + m_localPosition.y, MEMORY_SIZE);

	m_tiles[IndexIntoTilemap(x, y)] = DataTile::FromTile(location.GetTile());
}

void TileMemory::SetTileByLocal(int x, int y, const Tile& tile)
{
	x = ModulusNegative<int>(x + m_localPosition.x, MEMORY_SIZE);
	y = ModulusNegative<int>(y + m_localPosition.y, MEMORY_SIZE);

	m_tiles[IndexIntoTilemap(x, y)] = DataTile::FromTile(tile);
}

void TileMemory::SetTileByLocal(int x, int y, const DataTile& tile)
{
	x = ModulusNegative<int>(x + m_localPosition.x, MEMORY_SIZE);
	y = ModulusNegative<int>(y + m_localPosition.y, MEMORY_SIZE);

	m_tiles[IndexIntoTilemap(x, y)] = tile;
}

bool TileMemory::ValidTile(int x, int y)
{
	x = ModulusNegative<int>(x + m_localPosition.x, MEMORY_SIZE);
	y = ModulusNegative<int>(y + m_localPosition.y, MEMORY_SIZE);

	return (x >= 0 && x < MEMORY_SIZE && y >= 0 && y < MEMORY_SIZE);
}

DataTile& TileMemory::GetTileByLocal(int x, int y)
{
	ASSERT(ValidTile(x, y));
	x = ModulusNegative<int>(x + m_localPosition.x, MEMORY_SIZE);
	y = ModulusNegative<int>(y + m_localPosition.y, MEMORY_SIZE);

	return m_tiles[IndexIntoTilemap(x, y)];
}

int TileMemory::IndexIntoTilemap(short x, short y)
{
	int bigX = x / INTERLEAVE_VALUE;
	int lilX = x % INTERLEAVE_VALUE;
	int bigY = y / INTERLEAVE_VALUE;
	int lilY = y % INTERLEAVE_VALUE;

	int bigValue = (bigX + bigY * (MEMORY_SIZE / (INTERLEAVE_VALUE))) * (INTERLEAVE_VALUE * INTERLEAVE_VALUE);
	int lilValue = lilX + (lilY * INTERLEAVE_VALUE);

	return bigValue + lilValue;
	//return (x + (MEMORY_SIZE * y));
}

void TileMemory::Debug_CheckIndexingDuplicates()
{
	std::unordered_map<int, Vec2> indicesMap;
	for (int j = 0; j < MEMORY_SIZE; j++)
	{
		for (int i = 0; i < MEMORY_SIZE; i++)
		{
			int index = IndexIntoTilemap(i, j);

			if (indicesMap.contains(index))
			{
				Vec2 match = indicesMap[index];
				DEBUG_PRINT("Duplicate indices!! [%d, %d] and [%d, %d]", match.x, match.y, i, j);
			}
			else
			{
				indicesMap[index] = Vec2(i, j);
			}
		}
	}
}

void TileMemory::WipeLocalRect(Vec2 position, Vec2 size)
{
	for (int xOff = 0; xOff < size.x; xOff++)
	{
		for (int yOff = 0; yOff < size.y; yOff++)
		{
			int x = ModulusNegative<int>(position.x + xOff + m_localPosition.x, MEMORY_SIZE);
			int y = ModulusNegative<int>(position.y + yOff + m_localPosition.y, MEMORY_SIZE);

			m_tiles[IndexIntoTilemap(x, y)] = DataTile();
		}
	}
}

Vec2 TileMemory::WrapVector(Vec2 vector)
{
	Vec2 outVector;
	vector.x = ModulusNegative<short>(vector.x, MEMORY_SIZE);
	vector.y = ModulusNegative<short>(vector.y, MEMORY_SIZE);
	return outVector;
}