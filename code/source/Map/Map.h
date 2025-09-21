#pragma once
#include "Data/RogueDataManager.h"
#include "Core/CoreDataTypes.h"
#include "Core/Materials/Materials.h"
#include <type_traits>

class BackingTile;
class TileStats;
class TileNeighbors;
class Tile;
class Location;

class BackingTile
{
public:
    BackingTile() {}
    BackingTile(char character, Color foreground, Color background, bool blocksVision, MaterialContainer floorMaterials, MaterialContainer volumeMaterials) :
        m_renderCharacter(character),
        m_foregroundColor(foreground),
        m_backgroundColor(background),
        m_blocksVision(blocksVision),
        m_defaultFloorMaterials(floorMaterials),
        m_defaultVolumeMaterials(volumeMaterials)
    {
        ASSERT(m_defaultVolumeMaterials.m_inverted);
        m_defaultVolumeMaterials.m_inverted = true;
    }

	char m_renderCharacter;
    Color m_foregroundColor;
    Color m_backgroundColor;
	bool m_blocksVision;
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
    float m_heat;

    bool operator==(const Tile& other);
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
        THandle<BackingTile> tile = Game::dataManager->Allocate<T>(std::forward<Args>(args)...);
        return LinkBackingTile(tile);
    }
};

namespace Serialization
{
    template<typename Stream>
    void Serialize(Stream& stream, const BackingTile& value)
    {
        Write(stream, "Char", value.m_renderCharacter);
        Write(stream, "FGColor", value.m_foregroundColor);
        Write(stream, "BGColor", value.m_backgroundColor);
        Write(stream, "Blocks Vision", value.m_blocksVision);
        Write(stream, "Default Floor Materials", value.m_defaultFloorMaterials);
        Write(stream, "Default Volume Materials", value.m_defaultVolumeMaterials);
        Write(stream, "Index", value.m_index);
    }

    template<typename Stream>
    void Deserialize(Stream& stream, BackingTile& value)
    {
        Read(stream, "Char", value.m_renderCharacter);
        Read(stream, "FGColor", value.m_foregroundColor);
        Read(stream, "BGColor", value.m_backgroundColor);
        Read(stream, "Blocks Vision", value.m_blocksVision);
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
    void Serialize(Stream& stream, const Tile& value)
    {
        Write(stream, "Backing", value.m_backingTile);
        Write(stream, "Stats", value.m_stats);
        Write(stream, "Heat", value.m_heat);
    }

    template <typename Stream>
    void Deserialize(Stream& stream, Tile& value)
    {
        Read(stream, "Backing", value.m_backingTile);
        Read(stream, "Stats", value.m_stats);
        Read(stream, "Heat", value.m_heat);
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