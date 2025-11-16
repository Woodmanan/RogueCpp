#pragma once
#include "Core/CoreDataTypes.h"
#include <optional>

class Chunk;
class BackingTile;

class SeededGenerator {
public:
	SeededGenerator(uint seed) : m_seed(seed) {}

	private:
		uint m_seed;
};

class IWorldValueProvider
{
public:
	virtual float GetValue(Vec3 worldPosition) = 0;
};

class IWorldBoundsProvider
{
public:
	virtual std::optional<Rect3> GetBounds(Vec3 worldPosition) = 0;
};

class IWorldTileProvider
{
public:
	virtual Tile GetTile(Vec3 tilePosition) = 0;
};

class SimplexNoiseProvider : public SeededGenerator, public IWorldValueProvider
{
public:
	SimplexNoiseProvider(uint seed) : SeededGenerator(seed) {}

	virtual float GetValue(Vec3 worldPosition) override;
};

class ValueToTileConverter : public IWorldTileProvider
{
public:
	ValueToTileConverter(IWorldValueProvider* provider, std::vector<THandle<BackingTile>> backingTiles) : m_provider(provider), m_backingTiles(backingTiles) {}

	virtual Tile GetTile(Vec3 tilePosition) override;
private:
	IWorldValueProvider* m_provider;
	std::vector<THandle<BackingTile>> m_backingTiles;
};

class WorldManager
{
public:
	WorldManager();
	void Init();
	
	Chunk* LoadChunk(Vec3 chunkPosition);

private:
	IWorldTileProvider* m_rootProvider;
};