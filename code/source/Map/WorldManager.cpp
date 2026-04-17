#include "WorldManager.h"
#include "Map.h"
#include "Game/Game.h"
#include "Core/Math/Perlin.h"

float SimplexNoiseProvider::GetValue(Vec4 worldPosition)
{
    double x = worldPosition.x * 0.05f;
    double y = worldPosition.y* 0.05f;
    double z = worldPosition.z * 0.05f;

    return Perlin::OctavePerlin(x, y, z, 5, .5f);
}

Tile ValueToTileConverter::GetTile(Vec4 tilePosition)
{
    Tile tile;
    float value = m_provider->GetValue(tilePosition);

    if (value < .4f)
    {
        tile.m_backingTile = m_backingTiles[0];
    }
    else
    {
        tile.m_backingTile = m_backingTiles[1];
    }
    tile.m_wall = tile.GetVisibleMaterial().second;

    return tile;
}

WorldManager::WorldManager()
{

}

void WorldManager::Init()
{
    SimplexNoiseProvider* noise = new SimplexNoiseProvider(0);
    MaterialContainer groundMat;
    groundMat.AddMaterial("Stone", 1000);

    MaterialContainer mudMat;
    mudMat.AddMaterial("Mud", 1000);

    MaterialContainer airMat(true);
    airMat.AddMaterial("Air", 1);

    MaterialContainer woodWallMat(true);
    woodWallMat.AddMaterial("Wood", 1000, true);

    MaterialContainer stoneWallMat(true);
    stoneWallMat.AddMaterial("Stone", 1000, true);

    MaterialContainer dirtMat;
    dirtMat.AddMaterial("Dirt", 1000, true);

    THandle<BackingTile> wall = Game::dataManager->Allocate<BackingTile>(groundMat, woodWallMat);
    THandle<BackingTile> floor = Game::dataManager->Allocate<BackingTile>(mudMat, airMat);

    std::vector<THandle<BackingTile>> tiles;
    tiles.push_back(wall);
    tiles.push_back(floor);

    ValueToTileConverter* converter = new ValueToTileConverter(noise, tiles);

    m_rootProvider = converter;
}

Chunk* WorldManager::LoadChunk(Vec4 chunkPosition)
{
    ROGUE_PROFILE_SECTION("WorldManager::LoadChunk");
    Chunk* newChunk = new Chunk(chunkPosition);
    Vec4 chunkCorner = chunkPosition * Vec4(CHUNK_SIZE_X, CHUNK_SIZE_Y, CHUNK_SIZE_Z, CHUNK_SIZE_W);

    for (int x = 0; x < CHUNK_SIZE_X; x++)
    {
        for (int y = 0; y < CHUNK_SIZE_Y; y++)
        {
            for (int z = 0; z < CHUNK_SIZE_Z; z++)
			{
				for (int w = 0; w < CHUNK_SIZE_W; w++)
				{
					Vec4 localPos = Vec4(x, y, z);
					Vec4 worldPos = chunkCorner + localPos;
					newChunk->SetTile(localPos, m_rootProvider->GetTile(worldPos));
				}
			}
        }
    }

    //Set default heat values
    float defaultHeat = -4.f;
    newChunk->SetDefaultHeat(defaultHeat);
    for (int x = 0; x < CHUNK_SIZE_X; x++)
    {
        for (int y = 0; y < CHUNK_SIZE_Y; y++)
        {
            for (int z = 0; z < CHUNK_SIZE_Z; z++)
            {
	            for (int w = 0; w < CHUNK_SIZE_W; w++)
            	{
                	newChunk->GetTile(Vec4(x, y, z, w)).m_heat = defaultHeat;
            	}
			}
        }
    }

    return newChunk;
}
