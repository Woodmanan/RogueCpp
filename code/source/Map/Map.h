#pragma once
#include "Data/RogueDataManager.h"
#include "Core/CoreDataTypes.h"
#include "Core/Materials/Materials.h"
#include <type_traits>
#include <unordered_map>
#include <set>

class BackingTile;
class TileStats;
class TileNeighbors;
class Tile;
class Location;
class Chunk;
class ChunkMap;

namespace Serialization
{
    template<typename Stream>
    void Serialize(Stream& stream, const Chunk& value);
    template<typename Stream>
    void Deserialize(Stream& stream, Chunk& value);

    template<typename Stream>
    void Serialize(Stream& stream, const ChunkMap& value);
    template<typename Stream>
    void Deserialize(Stream& stream, ChunkMap& value);
}

//Static map data!
//Chunk size defined in CoreDataTypes, since other systems might depend upon it
static constexpr int ACTIVE_CHUNK_RADIUS = 8;
static constexpr int LOAD_CHUNK_RADIUS = 16;

//TODO: Unload radius!

class BackingTile
{
public:
    BackingTile() {}
    BackingTile(MaterialContainer floorMaterials, MaterialContainer volumeMaterials) :
        m_defaultFloorMaterials(floorMaterials),
        m_defaultVolumeMaterials(volumeMaterials)
    {
        ASSERT(m_defaultVolumeMaterials.m_inverted);
        m_defaultVolumeMaterials.m_inverted = true;
    }

    MaterialContainer m_defaultFloorMaterials; //Materials that make up the floor
    MaterialContainer m_defaultVolumeMaterials; //Materials that fill the standing volume of the tile
    int m_index = -1;

    bool operator==(const BackingTile& other) const = default;
    bool operator!=(const BackingTile& other) const = default;
};

class TileStats
{
public:
    THandle<TileNeighbors> m_neighbors;
    MaterialContainer m_floorMaterials; //Materials that make up the floor
    MaterialContainer m_volumeMaterials; //Materials that fill the standing volume of the tile
};

class TileNeighbors
{
public:
    Location N;
    Direction N_Direction;
    Location NE;
    Direction NE_Direction;
    Location E;
    Direction E_Direction;
    Location SE;
    Direction SE_Direction;
    Location S;
    Direction S_Direction;
    Location SW;
    Direction SW_Direction;
    Location W;
    Direction W_Direction;
    Location NW;
    Direction NW_Direction;
};

class Tile
{
public:
    Tile() : m_heat(0) {}
    Tile(float baseHeat) : m_heat(baseHeat) {}

	THandle<BackingTile> m_backingTile;
	THandle<TileStats> m_stats;
    bool m_wall;
    float m_heat;
    bool m_dirty = false;

    bool operator==(const Tile& other);

    //Flyweight pattern - check if we have had to create data and create it when needed
    bool UsingInstanceData() const;
    void CreateInstanceData();
    pair<int, bool> GetVisibleMaterial() const;
};

class Chunk
{
public:
    Chunk() {}
    Chunk(Vec3 chunkLocation);
    Tile& GetTile(Vec3 location);
    void SetTile(Vec3 location, THandle<BackingTile> tile);
    void SetTile(Vec3 location, const Tile& tile);

    Vec3 GetChunkCorner() const;

    void GenerateHeatDeltas(int timeStep, vector<float>& scratch);
    bool ApplyHeatDeltas(vector<float>& scratch);

    void SetDefaultHeat(float heat) { m_defaultHeat = heat; }
    bool GetDirty() { return m_dirty; }
    void MarkDirty();
    void ClearDirty() { m_dirty = false; }


private:
    int GetIndex(const Vec3& location);

    template<typename Stream>
    friend void Serialization::Serialize(Stream& stream, const Chunk& value);
    template<typename Stream>
    friend void Serialization::Deserialize(Stream& stream, Chunk& value);

    Vec3 m_chunkLocation;
    vector<Tile> m_tiles;
    float m_defaultHeat = 0;
    bool m_dirty = false;
};

class ChunkMap
{
public:
    void TriggerStreamingAroundLocation(Location loc, Vec3 radius = Vec3(LOAD_CHUNK_RADIUS, LOAD_CHUNK_RADIUS, 0));
    void WaitForStreaming();
    Tile& GetTile(Location location);
    void AsyncAddChunk(Vec3 chunkLoc, Chunk* chunk);

    int LinkBackingTile(THandle<BackingTile> tile);
    template<typename T, class... Args>
    int LinkBackingTile(Args&&... args)
    {
        static_assert(std::is_convertible<T*, BackingTile*>::value, "T must inherit from BackingTile");
        THandle<BackingTile> tile = GetDataManager()->Allocate<T>(std::forward<Args>(args)...);
        return LinkBackingTile(tile);
    }

    void SetTile(Location location, int index);
    void SetTile(Location location, THandle<BackingTile> tile);

    void Simulate(Location location, Vec3 radius = Vec3(ACTIVE_CHUNK_RADIUS, ACTIVE_CHUNK_RADIUS, 0));

    void AddHeat(Location location, float heat);

private:
    Chunk* GetChunk(Vec3 chunkId);
    void StreamChunk(Vec3 chunkId, Vec3 radius);
    void MainThread_InsertReadyChunks();

    void LoadChunk(Vec3 chunk);

    template<typename Stream>
    friend void Serialization::Serialize(Stream& stream, const ChunkMap& value);
    template<typename Stream>
    friend void Serialization::Deserialize(Stream& stream, ChunkMap& value);

    std::mutex m_mapMutex;
    unordered_map<Vec3, Chunk*> m_chunks;
    unordered_map<Vec3, Chunk*> m_readyChunks;
    set<Vec3> m_loadingChunks;
    vector<THandle<BackingTile>> m_backingTiles;
    vector<vector<float>> m_heatScratch;
};

class Map
{
public:
    Map() {}
    Map(Vec2 size, int mapZ, float defaultHeat, int interleave = 2) : m_size(size), m_interleaveBits(interleave), m_defaultHeat(defaultHeat)
    {
        ASSERT(size.x % (1 << (interleave)) == 0);
        ASSERT(size.y % (1 << (interleave)) == 0);

        m_tiles.resize(size.x * size.y, Tile(defaultHeat));
        z = mapZ;
    }

    Vec2 m_size;
    int m_interleaveBits;
    int z;
    float m_defaultHeat;

    vector<THandle<BackingTile>> m_backingTiles;
    vector<Tile> m_tiles;

    int IndexIntoMap(Vec2 location) const;
    int IndexIntoMap(short x, short y) const;

    Tile& GetTile(Vec2 location);
    Tile& GetTile(ushort x, ushort y);
    const Tile& GetTile(Vec2 location) const;
    const Tile& GetTile(ushort x, ushort y) const;

    void SetTile(Vec2 location, int index);
    void SetTile(Vec2 location, THandle<BackingTile> tile);

    void FillTilesInc(Vec2 from, Vec2 to, int index);
    void FillTilesExc(Vec2 from, Vec2 to, int index);

    void Reset();

    THandle<TileStats> GetOrAddStats(Tile& tile);
    THandle<TileNeighbors> GetOrAddNeighbors(Tile& tile);
    void SetNeighbors(Tile& tile, TileNeighbors neighbors);

    void WrapMapEdges();
    void WrapTile(Vec2 location);
    Location WrapVector(Vec2 location, int xOffset, int yOffset);
    Location WrapLocation(Location location, int xOffset, int yOffset);
    void SetNeighbor(Vec2 location, Direction direction, Location neighbor, Direction rotation = North);
    Location GetNeighbor(Vec2 location, Direction direction);

    void CreatePortal(Vec2 open, Vec2 exit);
    void CreateDirectionalPortal(Vec2 open, Vec2 exit, Direction direction);
    void CreateBidirectionalPortal(Vec2 open, Direction openDir, Vec2 exit, Direction exitDir);
    
    //Simulation controls
public:
    void Simulate();
private:
    void RunSimulationStep(Vec2 location, int timeStep, vector<float>& heatScratch);

public:
    int LinkBackingTile(THandle<BackingTile> tile);
    template<typename T, class... Args>
    int LinkBackingTile(Args&&... args)
    {
        static_assert(std::is_convertible<T*, BackingTile*>::value, "T must inherit from BackingTile");
        THandle<BackingTile> tile = GetDataManager()->Allocate<T>(std::forward<Args>(args)...);
        return LinkBackingTile(tile);
    }
};

namespace Serialization
{
    template<typename Stream>
    void Serialize(Stream& stream, const BackingTile& value)
    {
        Write(stream, "Default Floor Materials", value.m_defaultFloorMaterials);
        Write(stream, "Default Volume Materials", value.m_defaultVolumeMaterials);
        Write(stream, "Index", value.m_index);
    }

    template<typename Stream>
    void Deserialize(Stream& stream, BackingTile& value)
    {
        Read(stream, "Default Floor Materials", value.m_defaultFloorMaterials);
        Read(stream, "Default Volume Materials", value.m_defaultVolumeMaterials);
        Read(stream, "Index", value.m_index);
    }

    template<typename Stream>
    void Serialize(Stream& stream, const TileStats& value)
    {
        Write(stream, "Neighbors", value.m_neighbors);
        Write(stream, "Floor", value.m_floorMaterials);
        Write(stream, "Volume", value.m_volumeMaterials);
    }

    template <typename Stream>
    void Deserialize(Stream& stream, TileStats& value)
    {
        Read(stream, "Neighbors", value.m_neighbors);
        Read(stream, "Floor", value.m_floorMaterials);
        Read(stream, "Volume", value.m_volumeMaterials);
    }

    template<typename Stream>
    void Serialize(Stream& stream, const Map& value)
    {
        Write(stream, "Z", value.z);
        Write(stream, "Size", value.m_size);
        Write(stream, "Interleave", value.m_interleaveBits);
        Write(stream, "Default Heat", value.m_defaultHeat);
        Write(stream, "Backing Tiles", value.m_backingTiles);
        if constexpr (std::is_same<Stream, JSONStream>::value)
        {
            for (int j = 0; j < value.m_size.y; j++)
            {
                for (int i = 0; i < value.m_size.x; i++)
                {
                    Vec2 location(i, j);
                    Write(stream, "Location", location);
                    Write(stream, "Tile", value.GetTile(location));
                }
            }
        }
        else
        {
            Write(stream, "Tiles", value.m_tiles);
        }
    }

    template <typename Stream>
    void Deserialize(Stream& stream, Map& value)
    {
        Read(stream, "Z", value.z);
        Read(stream, "Size", value.m_size);
        Read(stream, "Interleave", value.m_interleaveBits);
        Read(stream, "Default Heat", value.m_defaultHeat);
        Read(stream, "Backing Tiles", value.m_backingTiles);
        value.m_tiles = std::vector<Tile>(value.m_size.x * value.m_size.y, Tile());
        if constexpr (std::is_same<Stream, JSONStream>::value)
        {
            for (int j = 0; j < value.m_size.y; j++)
            {
                for (int i = 0; i < value.m_size.x; i++)
                {
                    Vec2 location(i, j);
                    Read<Stream, Vec2>(stream, "Location");
                    Read(stream, "Tile", value.GetTile(location));
                }
            }
        }
        else
        {
            Read(stream, "Tiles", value.m_tiles);
        }
    }

    template<typename Stream>
    void Serialize(Stream& stream, const Chunk& value)
    {
        Write(stream, "Chunk Location", value.m_chunkLocation);
        Write(stream, "Tiles", value.m_tiles);
        Write(stream, "Default Heat", value.m_defaultHeat);
        Write(stream, "Dirty", value.m_dirty);
    }

    template <typename Stream>
    void Deserialize(Stream& stream, Chunk& value)
    {
        Read(stream, "Chunk Location", value.m_chunkLocation);
        Read(stream, "Tiles", value.m_tiles);
        Read(stream, "Default Heat", value.m_defaultHeat);
        Read(stream, "Dirty", value.m_dirty);
    }

    template<typename Stream>
    void Serialize(Stream& stream, const ChunkMap& value)
    {
        uint32_t chunkCount = value.m_chunks.size();
        Write(stream, "Chunk Count", chunkCount);

        for (auto it : value.m_chunks)
        {
            Write(stream, "Location", it.first);
            Write(stream, "Chunk", *it.second);
        }

        Write(stream, "Backing Tiles", value.m_backingTiles);
    }

    template <typename Stream>
    void Deserialize(Stream& stream, ChunkMap& value)
    {
        uint chunkCount;
        Read(stream, "Chunk Count", chunkCount);

        for (uint count = 0; count < chunkCount; count++)
        {
            Vec3 location;
            Chunk* chunk = new Chunk();
            Read(stream, "Location", location);
            Read(stream, "Chunk", *chunk);

            value.m_chunks[location] = chunk;
        }

        Read(stream, "Backing Tiles", value.m_backingTiles);
    }

    template<typename Stream>
    void Serialize(Stream& stream, const Tile& value)
    {
        Write(stream, "Backing", value.m_backingTile);
        Write(stream, "Stats", value.m_stats);
        Write(stream, "Heat", value.m_heat);
        Write(stream, "Wall", value.m_wall);
        Write(stream, "Dirty", value.m_dirty);
    }

    template <typename Stream>
    void Deserialize(Stream& stream, Tile& value)
    {
        Read(stream, "Backing", value.m_backingTile);
        Read(stream, "Stats", value.m_stats);
        Read(stream, "Heat", value.m_heat);
        Read(stream, "Wall", value.m_wall);
        Read(stream, "Dirty", value.m_dirty);
    }

    template<typename Stream>
    void Serialize(Stream& stream, const TileNeighbors& value)
    {
        Write(stream, "N", value.N);
        Write(stream, "NDir", value.N_Direction);
        Write(stream, "NE", value.NE);
        Write(stream, "NEDir", value.NE_Direction);
        Write(stream, "E", value.E);
        Write(stream, "EDir", value.E_Direction);
        Write(stream, "SE", value.SE);
        Write(stream, "SEDir", value.SE_Direction);
        Write(stream, "S", value.S);
        Write(stream, "SDir", value.S_Direction);
        Write(stream, "SW", value.SW);
        Write(stream, "SWDir", value.SW_Direction);
        Write(stream, "W", value.W);
        Write(stream, "WDir", value.W_Direction);
        Write(stream, "NW", value.NW);
        Write(stream, "NWDir", value.NW_Direction);
    }

    template <typename Stream>
    void Deserialize(Stream& stream, TileNeighbors& value)
    {
        Read(stream, "N", value.N);
        Read(stream, "NDir", value.N_Direction);
        Read(stream, "NE", value.NE);
        Read(stream, "NEDir", value.NE_Direction);
        Read(stream, "E", value.E);
        Read(stream, "EDir", value.E_Direction);
        Read(stream, "SE", value.SE);
        Read(stream, "SEDir", value.SE_Direction);
        Read(stream, "S", value.S);
        Read(stream, "SDir", value.S_Direction);
        Read(stream, "SW", value.SW);
        Read(stream, "SWDir", value.SW_Direction);
        Read(stream, "W", value.W);
        Read(stream, "WDir", value.W_Direction);
        Read(stream, "NW", value.NW);
        Read(stream, "NWDir", value.NW_Direction);
    }
}