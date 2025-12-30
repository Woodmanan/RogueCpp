#include "Map.h"
#include "Game/Game.h"
#include "Data/JobSystem.h"
#include "Core/Collections/StackArray.h"

bool Tile::operator==(const Tile& other)
{
    return (m_backingTile == other.m_backingTile) && (m_stats == other.m_stats) && (m_heat == other.m_heat);
}

bool Tile::UsingInstanceData() const
{
    return m_stats.IsValid();
}

void Tile::CreateInstanceData()
{
    ASSERT(!UsingInstanceData());
    m_stats = GetDataManager()->Allocate<TileStats>();
    m_stats->m_floorMaterials = MaterialContainer(m_backingTile->m_defaultFloorMaterials);
    m_stats->m_volumeMaterials = MaterialContainer(m_backingTile->m_defaultVolumeMaterials);
}

pair<int, bool> Tile::GetVisibleMaterial() const
{
    if (UsingInstanceData())
    {
        for (int i = 0; i < m_stats->m_volumeMaterials.m_materials.size(); i--)
        {
            const Material& mat = m_stats->m_volumeMaterials.m_materials[i];
            if (mat.GetMaterial().wallChar != '\0')
            {
                return { mat.m_materialID, true };
            }
        }

        for (int i = m_stats->m_floorMaterials.m_materials.size() - 1; i >= 0; i--)
        {
            const Material& mat = m_stats->m_floorMaterials.m_materials[i];
            if (mat.GetMaterial().floorChar != '\0')
            {
                return { mat.m_materialID, false };
            }
        }

        return { -1, false };
    }
    else
    {
        for (int i = 0; i < m_backingTile->m_defaultVolumeMaterials.m_materials.size(); i--)
        {
            const Material& mat = m_backingTile->m_defaultVolumeMaterials.m_materials[i];
            if (mat.GetMaterial().wallChar != '\0')
            {
                return { mat.m_materialID, true };
            }
        }

        for (int i = m_backingTile->m_defaultFloorMaterials.m_materials.size() - 1; i >= 0; i--)
        {
            const Material& mat = m_backingTile->m_defaultFloorMaterials.m_materials[i];
            if (mat.GetMaterial().floorChar != '\0')
            {
                return { mat.m_materialID, false };
            }
        }

        return { -1, false };
    }
}

void Tile::BreakWall()
{
    ASSERT(m_wall);

    if (!UsingInstanceData())
    {
        CreateInstanceData();
    }

    STACKARRAY(Material, materialsToMove, 20);
    for (const Material& material : m_stats->m_volumeMaterials.m_materials)
    {
        MaterialDefinition definition = GetMaterialManager()->GetMaterialDefinition(material);
        if (definition.phase == Phase::Solid)
        {
            materialsToMove.push_back(material);
        }
    }

    for (const Material& material : materialsToMove)
    {
        m_stats->m_volumeMaterials.RemoveMaterial(material);
        m_stats->m_floorMaterials.AddMaterial(material);
    }

    m_wall = false;
    m_dirty = true;
}

Chunk::Chunk(Vec3 chunkLocation)
{
    m_chunkLocation = chunkLocation;
    m_tiles.resize(CHUNK_SIZE_X * CHUNK_SIZE_Y * CHUNK_SIZE_Z);
}

Tile& Chunk::GetTile(Vec3 location)
{
    return m_tiles[GetIndex(location)];
}

void Chunk::SetTile(Vec3 location, THandle<BackingTile> tile)
{
    Tile& mapTile = m_tiles[GetIndex(location)];
    mapTile.m_backingTile = tile;
    mapTile.m_wall = mapTile.GetVisibleMaterial().second;
}

void Chunk::SetTile(Vec3 location, const Tile& tile)
{
    Tile& mapTile = m_tiles[GetIndex(location)];
    mapTile = tile;
}

Vec3 Chunk::GetChunkCorner() const
{
    return m_chunkLocation * Vec3(CHUNK_SIZE_X, CHUNK_SIZE_Y, CHUNK_SIZE_Z);
}

void Chunk::GenerateHeatDeltas(int timeStep, vector<float>& scratch)
{
    for (int x = 0; x < CHUNK_SIZE_X; x++)
    {
        for (int y = 0; y < CHUNK_SIZE_Y; y++)
        {
            for (int z = 0; z < CHUNK_SIZE_Z; z++)
            {
                Vec3 localLocation = Vec3(x, y, z);
                Location location = Location(GetChunkCorner() + localLocation);

                int index = GetIndex(localLocation);

                Tile& tile = GetTile(localLocation);
                int materialIndex = tile.GetVisibleMaterial().first;
                const MaterialDefinition& material = GetMaterialManager()->GetMaterialByID(materialIndex);

                scratch[index] = 0;

                static constexpr float AirStrength = 0.2f; //0-1, how much the air heat loss is considered
                static constexpr float lossPerStep = .4f; //0-1, how much heat is transfered per step

                static constexpr float exponent = 1.0f - lossPerStep;
                float diff = 1 - std::pow(exponent, timeStep);

                for (Direction dir : {North, NorthEast, East, SouthEast, South, SouthWest, West, NorthWest})
                {
                    Location neighbor = location.GetNeighbor(dir);
                    const MaterialDefinition& otherMat = GetMaterialManager()->GetMaterialByID(neighbor->GetVisibleMaterial().first);

                    float avgConductivity = (material.thermalConductivity * otherMat.thermalConductivity);

                    float heatTransfer = (neighbor.GetTile().m_heat - tile.m_heat) * diff * 0.11f * avgConductivity;

                    if (abs(heatTransfer) > 0.01f)
                    {
                        scratch[index] += heatTransfer / material.heatCapacity;
                    }
                }

                float diffFromDefault = (m_defaultHeat - tile.m_heat) * diff * 0.11f * AirStrength;
                if (abs(diffFromDefault) > 0.01f)
                {
                    scratch[index] += diffFromDefault / material.heatCapacity;
                }
            }
        }
    }
}

bool Chunk::ApplyHeatDeltas(vector<float>& scratch)
{
    bool anyUpdates = false;
    for (int x = 0; x < CHUNK_SIZE_X; x++)
    {
        for (int y = 0; y < CHUNK_SIZE_Y; y++)
        {
            for (int z = 0; z < CHUNK_SIZE_Z; z++)
            {
                Vec3 localLocation = Vec3(x, y, z);
                Location location = Location(m_chunkLocation + localLocation);
                int index = GetIndex(localLocation);
                Tile& tile = GetTile(localLocation);

                float delta = scratch[index];
                if (delta != 0.0f)
                {
                    tile.m_heat += delta;
                    tile.m_dirty = true;
                    anyUpdates = true;
                }

                if (tile.m_dirty)
                {
                    tile.m_dirty = false;
                    if (!tile.UsingInstanceData())
                    {
                        if (Game::materialManager->CheckReaction(tile.m_backingTile->m_defaultFloorMaterials, tile.m_backingTile->m_defaultVolumeMaterials, tile.m_heat))
                        {
                            tile.CreateInstanceData();
                        }
                        else
                        {
                            continue;
                        }
                    }

                    if (Game::materialManager->EvaluateReaction(tile.m_stats->m_floorMaterials, tile.m_stats->m_volumeMaterials, tile.m_heat))
                    {
                        tile.m_wall = tile.GetVisibleMaterial().second;
                        tile.m_dirty = true;
                        anyUpdates = true;
                    }
                }
            }
        }
    }

    return anyUpdates;
}

void Chunk::MarkDirty()
{
    m_dirty = true;
}

int Chunk::GetIndex(const Vec3& location)
{
    ASSERT(location.x >= 0 && location.x < CHUNK_SIZE_X&& location.y >= 0 && location.y < CHUNK_SIZE_Y&& location.z >= 0 && location.z < CHUNK_SIZE_Z);
    int index = location.x + CHUNK_SIZE_X * location.y + CHUNK_SIZE_X * CHUNK_SIZE_Y * location.z;
    ASSERT(index >= 0 && index < m_tiles.size());
    return index;
}

void ChunkMap::TriggerStreamingAroundLocation(Location loc, Vec3 radius)
{
    ROGUE_PROFILE_SECTION("ChunkMap::Trigger Streaming");
    ASSERT(loc.GetValid());

    Vec3 chunkPosition = loc.GetChunkPosition();
    for (int x = -radius.x; x <= radius.x; x++)
    {
        for (int y = -radius.y; y <= radius.y; y++)
        {
            for (int z = -radius.z; z <= radius.z; z++)
            {
                Vec3 position = Vec3::WrapChunk(chunkPosition + Vec3(x, y, z));

                StreamChunk(position, Vec3(0, 0, 0));
            }
        }
    }

    MainThread_InsertReadyChunks();
}

void ChunkMap::WaitForStreaming()
{
    while (true)
    {
        bool anyWorkRemaining = !m_loadingChunks.empty();

        if (anyWorkRemaining)
        {
            MainThread_InsertReadyChunks();
        }
        else
        {
            break;
        }
    }
}

Tile& ChunkMap::GetTile(Location location)
{
    return GetChunk(location.GetChunkPosition())->GetTile(location.GetChunkLocalPosition());
}

void ChunkMap::AsyncAddChunk(Vec3 chunkLoc, Chunk* chunk)
{
    m_mapMutex.lock();
    ASSERT(!m_readyChunks.contains(chunkLoc));
    m_readyChunks[chunkLoc] = chunk;
    m_mapMutex.unlock();
}

int ChunkMap::LinkBackingTile(THandle<BackingTile> tile)
{
    m_backingTiles.push_back(tile);
    ASSERT(m_backingTiles.size() > 0);
    tile->m_index = m_backingTiles.size() - 1;
    return m_backingTiles.size() - 1;
}

void ChunkMap::SetTile(Location location, int index)
{
    SetTile(location, m_backingTiles[index]);
}

void ChunkMap::SetTile(Location location, THandle<BackingTile> tile)
{
    GetChunk(location.GetChunkPosition())->SetTile(location.GetChunkLocalPosition(), tile);
}

void ChunkMap::Simulate(Location location, Vec3 radius)
{
    ROGUE_PROFILE_SECTION("Chunk Map Simulation");

    int scratchIndex = 0;

    vector<Vec3> chunksToUpdate;
    Vec3 chunkPosition = location.GetChunkPosition();

    for (int x = -radius.x; x <= radius.x; x++)
    {
        for (int y = -radius.y; y <= radius.y; y++)
        {
            for (int z = -radius.z; z <= radius.z; z++)
            {
                Vec3 position = Vec3::WrapChunk(chunkPosition + Vec3(x, y, z));

                //Run simulation, write to scratch
                Chunk* chunk = GetChunk(position);

                if (chunk->GetDirty())
                {
                    if (scratchIndex >= m_heatScratch.size())
                    {
                        m_heatScratch.push_back(vector<float>(CHUNK_SIZE_X * CHUNK_SIZE_Y * CHUNK_SIZE_Z, 0.0f));
                    }

                    vector<float>& scratchPad = m_heatScratch[scratchIndex];
                    scratchIndex++;
                    chunk->GenerateHeatDeltas(1, scratchPad);
                }
            }
        }
    }
    
    scratchIndex = 0;
    for (int x = -radius.x; x <= radius.x; x++)
    {
        for (int y = -radius.y; y <= radius.y; y++)
        {
            for (int z = -radius.z; z <= radius.z; z++)
            {
                Vec3 position = Vec3::WrapChunk(chunkPosition + Vec3(x, y, z));

                //Write simulation results
                Chunk* chunk = GetChunk(position);

                if (chunk->GetDirty())
                {
                    if (scratchIndex >= m_heatScratch.size())
                    {
                        m_heatScratch.push_back(vector<float>(CHUNK_SIZE_X * CHUNK_SIZE_Y * CHUNK_SIZE_Z, 0.0f));
                    }

                    vector<float>& scratchPad = m_heatScratch[scratchIndex];
                    scratchIndex++;

                    bool anyUpdates = chunk->ApplyHeatDeltas(scratchPad);

                    if (anyUpdates)
                    {
                        for (int xOff = -1; xOff <= 1; xOff++)
                        {
                            for (int yOff = -1; yOff <= 1; yOff++)
                            {
                                chunksToUpdate.push_back(Vec3::WrapChunk(position + Vec3(xOff, yOff, 0)));
                            }
                        }
                    }
                    else
                    {
                        chunk->ClearDirty();
                    }
                }
            }
        }
    }

    for (Vec3 chunk : chunksToUpdate)
    {
        Chunk* chunkPtr = GetChunk(chunk);
        if (!chunkPtr->GetDirty())
        {
            chunkPtr->MarkDirty();
        }
    }
}

void ChunkMap::AddHeat(Location location, float heat)
{
    Tile& tile = GetTile(location);
    tile.m_heat += heat;

    Vec3 chunkPos = location.GetChunkPosition();

    for (int x = -1; x <= 1; x++)
    {
        for (int y = -1; y <= 1; y++)
        {
            GetChunk(Vec3::WrapChunk(chunkPos + Vec3(x, y, 0)))->MarkDirty();
        }
    }
}

Chunk* ChunkMap::GetChunk(Vec3 chunkId)
{
    if (!m_chunks.contains(chunkId))
    {
        //We need it! Enqueue it and wait. Stream with a small radius (we probably want it too)
        StreamChunk(chunkId, Vec3(1, 1, 0));
        while (!m_chunks.contains(chunkId))
        {
            MainThread_InsertReadyChunks();
        }
    }

    ASSERT(m_chunks.contains(chunkId));
    return m_chunks[chunkId];
}

void ChunkMap::StreamChunk(Vec3 chunkId, Vec3 radius)
{
    for (int x = -radius.x; x <= radius.x; x++)
    {
        for (int y = -radius.y; y <= radius.y; y++)
        {
            for (int z = -radius.z; z <= radius.z; z++)
            {
                Vec3 chunkPos = chunkId + Vec3(x, y, z);

                if (m_chunks.contains(chunkPos) || m_loadingChunks.contains(chunkPos))
                {
                    continue;
                }

                m_loadingChunks.insert(chunkPos);

                RogueDataManager* dataManager = Game::dataManager;
                MaterialManager* materialManager = Game::materialManager;
                WorldManager* worldManager = Game::worldManager;
                Jobs::QueueJob([this, chunkPos, dataManager, materialManager, worldManager]()
                    {
                        Game::dataManager = dataManager;
                        Game::materialManager = materialManager;
                        Game::worldManager = worldManager;
                        ASSERT(Game::dataManager != nullptr);
                        ASSERT(Game::materialManager != nullptr);
                        ASSERT(Game::worldManager != nullptr);
                        Chunk* chunk = worldManager->LoadChunk(chunkPos);
                        AsyncAddChunk(chunkPos, chunk);
                    });
            }
        }
    }
}

void ChunkMap::MainThread_InsertReadyChunks()
{
    m_mapMutex.lock();

    for (auto iterator : m_readyChunks)
    {
        const Vec3& chunk = iterator.first;
        //DEBUG_PRINT("Finished: [%d, %d, %d]", chunk.x, chunk.y, chunk.z);
        m_chunks[chunk] = iterator.second;
        m_loadingChunks.erase(chunk);
    }

    m_readyChunks.clear();
    m_mapMutex.unlock();
}
