#pragma once
#include "Core/CoreDataTypes.h"
#include "Core/Materials/Materials.h"
#include "Data/RogueDataManager.h"
#include "Debug/Profiling.h"
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

//Static map data!
//Chunk size defined in CoreDataTypes, since other systems might depend upon it
static constexpr int ACTIVE_CHUNK_RADIUS = 12;
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
    float m_movementCost = 1.0f;
    bool m_dirty = false;

    bool operator==(const Tile& other);

    //Flyweight pattern - check if we have had to create data and create it when needed
    
    pair<int, bool> GetVisibleMaterial() const;

	//Helpers
	bool UsingInstanceData() const;
    void CreateInstanceData();
};

class Chunk
{
public:
    Chunk() {}
    Chunk(Vec4 chunkLocation);
    Tile& GetTile(Vec4 location);
    void SetTile(Vec4 location, THandle<BackingTile> tile);
    void SetTile(Vec4 location, const Tile& tile);

    Vec4 GetChunkCorner() const;

    void GenerateHeatDeltas(int timeStep, vector<float>& scratch);
    bool ApplyHeatDeltas(vector<float>& scratch);

    void SetDefaultHeat(float heat) { m_defaultHeat = heat; }
    bool GetDirty() { return m_dirty; }
    void MarkDirty();
    void ClearDirty() { m_dirty = false; }


private:
    int GetIndex(const Vec4& location);

    Vec4 m_chunkLocation;
    vector<Tile> m_tiles;
    float m_defaultHeat = 0;
    bool m_dirty = false;

    friend struct Serialization::Serializer<Chunk>;
};

class ChunkMap
{
public:
    void TriggerStreamingAroundLocation(Location loc, Vec4 radius = Vec4(LOAD_CHUNK_RADIUS, LOAD_CHUNK_RADIUS, 0, 0));
    void WaitForStreaming();
    Tile& GetTile(Location location);
    void AsyncAddChunk(Vec4 chunkLoc, Chunk* chunk);

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

    void Simulate(Location location, Vec4 radius = Vec4(ACTIVE_CHUNK_RADIUS, ACTIVE_CHUNK_RADIUS, 0, 0));

    void AddHeat(Location location, float heat);

private:
    Chunk* GetChunk(Vec4 chunkId);
    void StreamChunk(Vec4 chunkId, Vec4 radius);
    void MainThread_InsertReadyChunks();

    ROGUE_LOCK(std::mutex, m_mapMutex);
    unordered_map<Vec4, Chunk*> m_chunks;
    unordered_map<Vec4, Chunk*> m_readyChunks;
    set<Vec4> m_loadingChunks;
    vector<THandle<BackingTile>> m_backingTiles;
    vector<vector<float>> m_heatScratch;

    friend struct Serialization::Serializer<ChunkMap>;
};

namespace Serialization
{
    template<>
    struct Serializer<BackingTile> : ObjectSerializer<BackingTile>
    {
    	template<typename Stream>
    	static void Serialize(Stream& stream, const BackingTile& value)
    	{
    	    Write(stream, "Default Floor Materials", value.m_defaultFloorMaterials);
    	    Write(stream, "Default Volume Materials", value.m_defaultVolumeMaterials);
    	    Write(stream, "Index", value.m_index);
    	}

    	template<typename Stream>
    	static void Deserialize(Stream& stream, BackingTile& value)
    	{
    	    Read(stream, "Default Floor Materials", value.m_defaultFloorMaterials);
    	    Read(stream, "Default Volume Materials", value.m_defaultVolumeMaterials);
    	    Read(stream, "Index", value.m_index);
    	}
    };
    
    template<>
    struct Serializer<TileStats> : ObjectSerializer<TileStats>
    {
    	template<typename Stream>
    	static void Serialize(Stream& stream, const TileStats& value)
    	{
    	    Write(stream, "Neighbors", value.m_neighbors);
    	    Write(stream, "Floor", value.m_floorMaterials);
    	    Write(stream, "Volume", value.m_volumeMaterials);
    	}

    	template <typename Stream>
    	static void Deserialize(Stream& stream, TileStats& value)
    	{
    	    Read(stream, "Neighbors", value.m_neighbors);
    	    Read(stream, "Floor", value.m_floorMaterials);
    	    Read(stream, "Volume", value.m_volumeMaterials);
    	}
    };

    template<>
    struct Serializer<Chunk> : ObjectSerializer<Chunk>
    {
    	template<typename Stream>
    	static void Serialize(Stream& stream, const Chunk& value)
    	{
    	    Write(stream, "Chunk Location", value.m_chunkLocation);
    	    Write(stream, "Tiles", value.m_tiles);
    	    Write(stream, "Default Heat", value.m_defaultHeat);
    	    Write(stream, "Dirty", value.m_dirty);
    	}

    	template <typename Stream>
    	static void Deserialize(Stream& stream, Chunk& value)
    	{
    	    Read(stream, "Chunk Location", value.m_chunkLocation);
    	    Read(stream, "Tiles", value.m_tiles);
    	    Read(stream, "Default Heat", value.m_defaultHeat);
    	    Read(stream, "Dirty", value.m_dirty);
    	}
    };

    template<>
    struct Serializer<ChunkMap> : ObjectSerializer<ChunkMap>
    {
    	template<typename Stream>
    	static void Serialize(Stream& stream, const ChunkMap& value)
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
    	static void Deserialize(Stream& stream, ChunkMap& value)
    	{
    	    uint chunkCount;
    	    Read(stream, "Chunk Count", chunkCount);

    	    for (uint count = 0; count < chunkCount; count++)
    	    {
    	        Vec4 location;
    	        Chunk* chunk = new Chunk();
    	        Read(stream, "Location", location);
    	        Read(stream, "Chunk", *chunk);

    	        value.m_chunks[location] = chunk;
    	    }

    	    Read(stream, "Backing Tiles", value.m_backingTiles);
    	}
    };

    template<>
    struct Serializer<Tile> : ObjectSerializer<Tile>
    {
    	template<typename Stream>
    	static void Serialize(Stream& stream, const Tile& value)
    	{
    	    Write(stream, "Backing", value.m_backingTile);
    	    Write(stream, "Stats", value.m_stats);
    	    Write(stream, "Heat", value.m_heat);
    	    Write(stream, "Wall", value.m_wall);
    	    Write(stream, "Dirty", value.m_dirty);
    	}

    	template <typename Stream>
    	static void Deserialize(Stream& stream, Tile& value)
    	{
    	    Read(stream, "Backing", value.m_backingTile);
    	    Read(stream, "Stats", value.m_stats);
    	    Read(stream, "Heat", value.m_heat);
    	    Read(stream, "Wall", value.m_wall);
    	    Read(stream, "Dirty", value.m_dirty);
    	}
    };

    template<>
    struct Serializer<TileNeighbors> : ObjectSerializer<TileNeighbors>
    {
    	template<typename Stream>
    	static void Serialize(Stream& stream, const TileNeighbors& value)
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
    	static void Deserialize(Stream& stream, TileNeighbors& value)
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
    };
}
