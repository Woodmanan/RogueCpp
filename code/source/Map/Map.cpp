#include "Map.h"
#include "Game/Game.h"
#include "Data/JobSystem.h"

#define INTERLEAVE
static const unsigned int masks[] = { 0x00, 0x01, 0x03, 0x07, 0x0F, 0x1F, 0x3F, 0x7F, 0xFF };

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

                static constexpr float AirStrength = 0.7f; //0-1, how much the air heat loss is considered
                static constexpr float lossPerStep = .3f; //0-1, how much heat is transfered per step

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
        m_mapMutex.lock();
        bool anyWorkRemaining = !m_loadingChunks.empty() || !m_readyChunks.empty();
        m_mapMutex.unlock();

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
    ASSERT(m_loadingChunks.contains(chunkLoc));
    ASSERT(!m_readyChunks.contains(chunkLoc));
    ASSERT(!m_chunks.contains(chunkLoc));
    m_readyChunks[chunkLoc] = chunk;
    m_loadingChunks.erase(chunkLoc);
    DEBUG_PRINT("Loaded [%d, %d, %d]: %d chunks loaded, %d chunks ready, %d chunks remaining", chunkLoc.x, chunkLoc.y, chunkLoc.z, m_chunks.size(), m_loadingChunks.size());
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
                Vec3 chunk = chunkId + Vec3(x, y, z);
                m_mapMutex.lock();
                if (m_chunks.contains(chunk) || m_loadingChunks.contains(chunk) || m_readyChunks.contains(chunk))
                {
                    m_mapMutex.unlock();
                    continue;
                }

                m_loadingChunks.insert(chunk);
                m_mapMutex.unlock();

                RogueDataManager* dataManager = Game::dataManager;
                MaterialManager* materialManager = Game::materialManager;
                Jobs::QueueJob([this, chunk, dataManager, materialManager]()
                    {
                        Game::dataManager = dataManager;
                        Game::materialManager = materialManager;
                        LoadChunk(chunk);
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
        m_chunks[iterator.first] = iterator.second;
    }

    m_readyChunks.clear();

    m_mapMutex.unlock();
}

void ChunkMap::LoadChunk(Vec3 chunk)
{
    ROGUE_PROFILE_SECTION("ChunkMap::LoadChunk");
    ASSERT(Game::dataManager != nullptr);
    Chunk* newChunk = new Chunk(chunk);

    if ((chunk.x + chunk.y) % 4 == 1)
    {
        for (int x = 0; x < CHUNK_SIZE_X; x++)
        {
            for (int y = 0; y < CHUNK_SIZE_Y; y++)
            {
                for (int z = 0; z < CHUNK_SIZE_Z; z++)
                {
                    if (x <= 1 || x > 6 || y <= 1 || y > 6)
                    {
                        newChunk->SetTile(Vec3(x, y, z), m_backingTiles[1]);
                    }
                    else
                    {
                        newChunk->SetTile(Vec3(x, y, z), m_backingTiles[0]);
                    }
                }
            }
        }
    }
    else
    {
        for (int x = 0; x < CHUNK_SIZE_X; x++)
        {
            for (int y = 0; y < CHUNK_SIZE_Y; y++)
            {
                for (int z = 0; z < CHUNK_SIZE_Z; z++)
                {
                    newChunk->SetTile(Vec3(x, y, z), m_backingTiles[1]);
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
                newChunk->GetTile(Vec3(x, y, z)).m_heat = defaultHeat;
            }
        }
    }

    AsyncAddChunk(chunk, newChunk);
}

int Map::IndexIntoMap(Vec2 location) const
{
    return IndexIntoMap(location.x, location.y);
}

int Map::IndexIntoMap(short x, short y) const
{
#ifdef INTERLEAVE
    int bigX = x >> m_interleaveBits;
    int bigY = y >> m_interleaveBits;
    int bigBlockIndex = (bigX + (m_size.x >> (m_interleaveBits)) * bigY) << (m_interleaveBits * 2);

    int lilX = x & masks[m_interleaveBits];
    int lilY = y & masks[m_interleaveBits];
    return bigBlockIndex + lilX + (1 << m_interleaveBits) * lilY;
#else // INTERLEAVE
	return (location.x + (m_size.x * location.y));
#endif
}

int Map::LinkBackingTile(THandle<BackingTile> tile)
{
	m_backingTiles.push_back(tile);
	ASSERT(m_backingTiles.size() > 0);
    tile->m_index = m_backingTiles.size() - 1;
	return m_backingTiles.size() - 1;
}

Tile& Map::GetTile(Vec2 location)
{
    return m_tiles[IndexIntoMap(location)];
}

Tile& Map::GetTile(ushort x, ushort y)
{
    return m_tiles[IndexIntoMap(x, y)];
}

const Tile& Map::GetTile(Vec2 location) const
{
    return m_tiles[IndexIntoMap(location)];
}
const Tile& Map::GetTile(ushort x, ushort y) const
{
    return m_tiles[IndexIntoMap(x, y)];
}

void Map::SetTile(Vec2 location, int index)
{
	SetTile(location, m_backingTiles[index]);
}

void Map::SetTile(Vec2 location, THandle<BackingTile> tile)
{
    Tile& mapTile = m_tiles[IndexIntoMap(location)];
    mapTile.m_backingTile = tile;
    mapTile.m_wall = mapTile.GetVisibleMaterial().second;
}

void Map::FillTilesInc(Vec2 from, Vec2 to, int index)
{
    ASSERT(from.x <= to.x && from.y <= to.y);
    for (int i = from.x; i <= to.x; i++)
    {
        for (int j = from.y; j <= to.y; j++)
        {
            SetTile(Vec2(i, j), index);
        }
    }
}

void Map::FillTilesExc(Vec2 from, Vec2 to, int index)
{
    ASSERT(from.x < to.x && from.y < to.y);
    for (int i = from.x; i < to.x; i++)
    {
        for (int j = from.y; j < to.y; j++)
        {
            SetTile(Vec2(i, j), index);
        }
    }
}

void Map::Reset()
{
    for (int i = 0; i < m_size.x; i++)
    {
        for (int j = 0; j < m_size.y; j++)
        {
            SetTile(Vec2(i, j), 0);
            Tile& tile = GetTile(i, j);
            if (tile.m_stats.IsValid())
            {
                WrapTile(Vec2(i, j));
            }
        }
    }
}

THandle<TileStats> Map::GetOrAddStats(Tile& tile)
{
    if (!tile.m_stats.IsValid())
    {
        tile.m_stats = GetDataManager()->Allocate<TileStats>();
    }

    ASSERT(tile.m_stats.IsValid());

    return tile.m_stats;
}

THandle<TileNeighbors> Map::GetOrAddNeighbors(Tile& tile)
{
    THandle<TileStats> stats = GetOrAddStats(tile);

    if (!stats->m_neighbors.IsValid())
    {
        stats->m_neighbors = GetDataManager()->Allocate<TileNeighbors>();
    }

    ASSERT(stats->m_neighbors.IsValid());

    return stats->m_neighbors;
}

void Map::SetNeighbors(Tile& tile, TileNeighbors neighbors)
{
    THandle<TileNeighbors> tileNeighbors = GetOrAddNeighbors(tile);
    tileNeighbors.GetReference() = neighbors;
}

void Map::WrapMapEdges()
{
    for (int x = 0; x < m_size.x; x++)
    {
        WrapTile(Vec2(x, 0));
        WrapTile(Vec2(x, m_size.y -1));
    }

    for (int y = 1; y < m_size.y - 1; y++)
    {
        WrapTile(Vec2(0, y));
        WrapTile(Vec2(m_size.x - 1, y));
    }
}

void Map::WrapTile(Vec2 location)
{
    Tile& tile = GetTile(location);
    THandle<TileNeighbors> neighbors = GetOrAddNeighbors(tile);
    ASSERT(tile.m_stats.IsValid() && neighbors.IsValid() && tile.m_stats->m_neighbors.GetInternalOffset() == neighbors.GetInternalOffset());

    neighbors->N  = WrapVector(location,  0,  1);
    neighbors->NE = WrapVector(location,  1,  1);
    neighbors->E  = WrapVector(location,  1,  0);
    neighbors->SE = WrapVector(location,  1, -1);
    neighbors->S  = WrapVector(location,  0, -1);
    neighbors->SW = WrapVector(location, -1, -1);
    neighbors->W  = WrapVector(location, -1,  0);
    neighbors->NW = WrapVector(location, -1,  1);

    neighbors->N_Direction = North;
    neighbors->NE_Direction = North;
    neighbors->E_Direction = North;
    neighbors->SE_Direction = North;
    neighbors->S_Direction = North;
    neighbors->SW_Direction = North;
    neighbors->W_Direction = North;
    neighbors->NW_Direction = North;
}

Location Map::WrapVector(Vec2 location, int xOffset, int yOffset)
{
    int x = location.x + xOffset;
    x = (x + m_size.x) % m_size.x;
    int y = location.y + yOffset;
    y = (y + m_size.y) % m_size.y;
    return Location(x, y, z);
}

Location Map::WrapLocation(Location location, int xOffset, int yOffset) {
    int x = location.x() + xOffset;
    x = (x + m_size.x) % m_size.x;
    int y = location.y() + yOffset;
    y = (y + m_size.y) % m_size.y;
    return Location(x, y, location.z());
}

void Map::SetNeighbor(Vec2 location, Direction direction, Location neighbor, Direction rotation)
{
    Tile& tile = GetTile(location);
    THandle<TileNeighbors> neighbors = GetOrAddNeighbors(tile);
    switch (direction)
    {
    case North:
        neighbors->N  = neighbor;
        neighbors->N_Direction = rotation;
        break;
    case NorthEast:
        neighbors->NE = neighbor;
        neighbors->NE_Direction = rotation;
        break;
    case East:
        neighbors->E  = neighbor;
        neighbors->E_Direction = rotation;
        break;
    case SouthEast:
        neighbors->SE = neighbor;
        neighbors->SE_Direction = rotation;
        break;
    case South:
        neighbors->S  = neighbor;
        neighbors->S_Direction = rotation;
        break;
    case SouthWest:
        neighbors->SW = neighbor;
        neighbors->SW_Direction = rotation;
        break;
    case West:
        neighbors->W  = neighbor;
        neighbors->W_Direction = rotation;
        break;
    case NorthWest:
        neighbors->NW = neighbor;
        neighbors->NW_Direction = rotation;
        break;
    }
}

Location Map::GetNeighbor(Vec2 location, Direction direction)
{
    Tile& tile = GetTile(location);
    THandle<TileNeighbors> neighbors = tile.m_stats.IsValid() ? tile.m_stats->m_neighbors : THandle<TileNeighbors>();

    if (neighbors.IsValid())
    {
        switch (direction)
        {
        case North:
            return neighbors->N;
        case NorthEast:
            return neighbors->NE;
        case East:
            return neighbors->E;
        case SouthEast:
            return neighbors->SE;
        case South:
            return neighbors->S;
        case SouthWest:
            return neighbors->SW;
        case West:
            return neighbors->W;
        case NorthWest:
            return neighbors->NW;
        }
    }
    else
    {
        Vec2 offset = Vector2FromDirection(direction);
        return WrapVector(location, offset.x, offset.y);
    }
}

void Map::CreatePortal(Vec2 open, Vec2 exit)
{
    WrapTile(open);
    WrapTile(exit);

    Tile& openTile = GetTile(open);
    Tile& exitTile = GetTile(exit);
    
    THandle<TileStats> openStats = GetOrAddStats(openTile);
    THandle<TileStats> exitStats = GetOrAddStats(exitTile);
    
    THandle<TileNeighbors> hold = openStats->m_neighbors;
    openStats->m_neighbors = exitStats->m_neighbors;
    exitStats->m_neighbors = hold;
}

void Map::CreateDirectionalPortal(Vec2 open, Vec2 exit, Direction direction)
{
    //Normalize neighbors
    WrapTile(open);
    WrapTile(exit);

    Direction CW = TurnClockwise(direction);
    Direction CCW = TurnCounterClockwise(direction);

    SetNeighbor(open, CW, GetNeighbor(exit, CW));
    SetNeighbor(open, direction, GetNeighbor(exit, direction));
    SetNeighbor(open, CCW, GetNeighbor(exit, CCW));

    SetNeighbor(exit, Reverse(CW), GetNeighbor(open, Reverse(CW)));
    SetNeighbor(exit, Reverse(direction), GetNeighbor(open, Reverse(direction)));
    SetNeighbor(exit, Reverse(CCW), GetNeighbor(open, Reverse(CCW)));
}

void Map::CreateBidirectionalPortal(Vec2 open, Direction openDir, Vec2 exit, Direction exitDir)
{
    WrapTile(open);
    WrapTile(exit);

    Direction rotation = FindRotationBetween(openDir, exitDir);

    Location exitLoc = GetNeighbor(exit, exitDir);
    Location exitLocCL = GetNeighbor(exit, TurnClockwise(exitDir));
    Location exitLocCCL = GetNeighbor(exit, TurnCounterClockwise(exitDir));

    Direction reverseOpen = Reverse(openDir);
    Direction reverseExit = Reverse(exitDir);
    Location openLoc = GetNeighbor(open, reverseOpen);
    Location openLocCL = GetNeighbor(open, TurnClockwise(reverseOpen));
    Location openLocCCL = GetNeighbor(open, TurnCounterClockwise(reverseOpen));

    SetNeighbor(open, openDir, exitLoc, rotation);
    SetNeighbor(open, TurnClockwise(openDir), exitLocCL, rotation);
    SetNeighbor(open, TurnCounterClockwise(openDir), exitLocCCL, rotation);

    SetNeighbor(exit, reverseExit, openLoc, ReverseRotation(rotation));
    SetNeighbor(exit, TurnClockwise(reverseExit), openLocCL, ReverseRotation(rotation));
    SetNeighbor(exit, TurnCounterClockwise(reverseExit), openLocCCL, ReverseRotation(rotation));
}

void Map::Simulate()
{
    ROGUE_PROFILE_SECTION("Map Simulation");
    static vector<float> heatScratch;
    if (heatScratch.size() < m_size.x * m_size.y)
    {
        heatScratch.resize(m_size.x * m_size.y);
    }

    for (int x = 0; x < m_size.x; x++)
    {
        for (int y = 0; y < m_size.y; y++)
        {
            heatScratch[IndexIntoMap(x, y)] = 0;
        }
    }

    for (int x = 0; x < m_size.x; x++)
    {
        for (int y = 0; y < m_size.y; y++)
        {
            RunSimulationStep(Vec2(x, y), 1, heatScratch);
        }
    }

    int numReactions = 0;
    for (int x = 0; x < m_size.x; x++)
    {
        for (int y = 0; y < m_size.y; y++)
        {
            float delta = heatScratch[IndexIntoMap(x, y)];
            Tile& tile = GetTile(x, y);
            if (delta != 0.0f)
            {
                tile.m_heat += delta;
                tile.m_dirty = true;
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
                    numReactions++;
                    tile.m_dirty = true;
                }
            }
        }
    }

    DEBUG_PRINT("Num reactions: %d", numReactions);
}

void Map::RunSimulationStep(Vec2 location, int timeStep, vector<float>& heatScratch)
{
    Tile& tile = GetTile(location);
    int materialIndex = tile.GetVisibleMaterial().first;
    const MaterialDefinition& material = GetMaterialManager()->GetMaterialByID(materialIndex);

    static constexpr float AirStrength = 0.05f; //0-1, how much the air heat loss is considered
    static constexpr float lossPerStep = .3f; //0-1, how much heat is transfered per step

    static constexpr float exponent = 1.0f - lossPerStep;
    float diff = 1 - std::pow(exponent, timeStep);

    for (Direction dir : {North, NorthEast, East, SouthEast, South, SouthWest, West, NorthWest})
    {
        Location neighbor = GetNeighbor(location, dir);
        const MaterialDefinition& otherMat = GetMaterialManager()->GetMaterialByID(neighbor->GetVisibleMaterial().first);

        float avgConductivity = (material.thermalConductivity + otherMat.thermalConductivity) * .5f;

        float heatTransfer = (neighbor.GetTile().m_heat - tile.m_heat) * diff * 0.11f * avgConductivity;

        if (abs(heatTransfer) > 0.01f)
        {
            heatScratch[IndexIntoMap(location)] += heatTransfer / material.heatCapacity;
        }
    }

    float diffFromDefault = (m_defaultHeat - tile.m_heat) * diff * 0.11f * AirStrength;
    if (abs(diffFromDefault) > 0.01f)
    {
        heatScratch[IndexIntoMap(location)] += diffFromDefault / material.heatCapacity;
    }
}
