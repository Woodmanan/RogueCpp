#pragma once
#include "../Data/SaveManager.h"
#include "../../libraries/BearLibTerminal/Include/C/BearLibTerminal.h"

class Tile;
class TileNeighbors;

template<typename T>
class THandle;

typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned int uint;

#ifdef _DEBUG
#define LINK_TILE
#endif

struct Vec2
{
    Vec2() : x(0), y(0) {}
	Vec2(short inX, short inY) : x(inX), y(inY) {}
	Vec2(int inX, int inY) : x((short)inX), y((short)inY) {}

	short x;
	short y;

    Vec2& operator+=(const Vec2& rhs) 
    {
        this->x += rhs.x;
        this->y += rhs.y;
        return *this;
    }
};

struct Vec3
{
    Vec3() : x(0), y(0), z(0) {}
    Vec3(short inX, short inY, short inZ) : x(inX), y(inY), z(inZ) {}
    Vec3(int inX, int inY, int inZ) : x((short)inX), y((short)inY), z((short)inZ) {}

    short x;
    short y;
    short z;
};

enum Direction
{
    North = 0,
    NorthEast = 1,
    East = 2,
    SouthEast = 3,
    South = 4,
    SouthWest = 5,
    West = 6,
    NorthWest = 7
};

std::istream& operator >> (std::istream& in, Direction& direction);

std::ostream& operator << (std::ostream& out, Direction direction);


inline static Direction Orthagonal(Direction direction)
{
    switch (direction)
    {
        case North:
        case South:
            return East;
        case East:
        case West:
            return North;
        case NorthEast:
        case SouthWest:
            return NorthWest;
        case NorthWest:
        case SouthEast:
            return NorthEast;
    }
}

inline static Direction Reverse(Direction direction)
{
    switch (direction)
    {
    case North:
        return South;
    case NorthEast:
        return SouthWest;
    case East:
        return West;
    case SouthEast:
        return NorthWest;
    case South:
        return North;
    case SouthWest:
        return NorthEast;
    case West:
        return East;
    case NorthWest:
        return SouthEast;
    }
}

inline static Direction TurnCounterClockwise(Direction direction)
{
    switch (direction)
    {
    case North:
        return NorthWest;
    case NorthEast:
        return North;
    case East:
        return NorthEast;
    case SouthEast:
        return East;
    case South:
        return SouthEast;
    case SouthWest:
        return South;
    case West:
        return SouthWest;
    case NorthWest:
        return West;
    }
}

inline static Direction TurnClockwise(Direction direction)
{
    switch (direction)
    {
    case North:
        return NorthEast;
    case NorthEast:
        return East;
    case East:
        return SouthEast;
    case SouthEast:
        return South;
    case South:
        return SouthWest;
    case SouthWest:
        return West;
    case West:
        return NorthWest;
    case NorthWest:
        return North;
    }
}

inline static Vec2 VectorFromDirection(const Direction direction)
{
    switch (direction)
    {
    case North:
        return Vec2( 0,  1);
    case NorthEast:
        return Vec2( 1,  1);
    case East:
        return Vec2( 1,  0);
    case SouthEast:
        return Vec2( 1, -1);
    case South:
        return Vec2( 0, -1);
    case SouthWest:
        return Vec2(-1, -1);
    case West:
        return Vec2(-1,  0);
    case NorthWest:
        return Vec2(-1,  1);
    }
}

class Location
{
public:
    Location();
    Location(ushort x, ushort y, ushort z);
    Location(Vec3 vector) : Location(vector.x, vector.y, vector.z) {};

    bool GetValid();
    void SetValid(bool valid);
    ushort x();
    ushort y();
    ushort z();

    Vec3 GetVector();
    void SetVector(Vec3 vector);

    Vec2 AsVec2();

    Tile& GetTile();
    Tile* operator ->();

#ifndef _DEBUG
    int GetData() { return m_data; }
    void SetData(int newData) { m_data = newData; }
#endif // _DEBUG

    bool InMap();

    Location Traverse(Vec2 offset);
    Location Traverse(short xOffset, short yOffset);
    Location Traverse(Direction direction);

    Location _Traverse_No_Neighbor(Direction direction);
    Location _Traverse_Neighbors(Direction direction);

    THandle<TileNeighbors> GetNeighbors();



    static const int xSize = 12;
    static const int ySize = 12;
    static const int zSize = 7;
    static const int xMask = (0x00000FFF) << (ySize + zSize);
    static const int yMask = 0x00000FFF << (zSize);
    static const int zMask = 0x0000007F;
    static const int invalidMask = 0x80000000;

#ifndef _DEBUG
private:
    int m_data;
#else
private:
    bool m_valid;
    ushort m_x;
    ushort m_y;
    ushort m_z;
#endif

#ifdef LINK_TILE
private:
    Tile* linkedTile = nullptr;
    void RefreshLinkedTile();
#endif
};

namespace RogueSaveManager
{
    void Serialize(Vec2& value);
    void Deserialize(Vec2& value);
    void Serialize(Vec3& value);
    void Deserialize(Vec3& value);
    void Serialize(Location& value);
    void Deserialize(Location& value);
}

//Define all the useful math ops here!
inline Location operator+(Location lhs, Vec2 rhs)
{
    int x = lhs.x() + rhs.x;
    int y = lhs.y() + rhs.y;
    if (x < 0 || y < 0)
    {
        return Location();
    }
    else
    {
        ASSERT(x < USHRT_MAX);
        ASSERT(y < USHRT_MAX);
        return Location((ushort)x, (ushort)y, lhs.z());
    }
}

//Define all the useful math ops here!
inline Location operator-(Location lhs, Vec2 rhs)
{
    int x = ((int)lhs.x()) - rhs.x;
    int y = ((int)lhs.y()) - rhs.y;
    ASSERT(x >= 0);
    ASSERT(y >= 0);
    return Location((ushort)x, (ushort)y, lhs.z());
}

inline Vec2 operator+(const Vec2 lhs, const Vec2 rhs)
{
    return Vec2(lhs.x + rhs.x, lhs.y + rhs.y);
}

inline Vec2 operator-(Vec2 lhs, Vec2 rhs)
{
    return Vec2(lhs.x - rhs.x, lhs.y - rhs.y);
}

inline int IntDivisionFloor(int num, int denom)
{
    if (num < 0)
    {
        int remainder = (num % denom) != 0;
        return num / denom - remainder;
    }
    else
    {
        return num / denom;
    }

}

inline int IntDivisionCeil(int num, int denom)
{
    if (num > 0)
    {
        int remainder = (num % denom) != 0;
        return num / denom + remainder;
    }
    else
    {
        return num / denom;
    }
}

template<typename T>
T Lerp(T a, T b, float percent)
{
    ASSERT(0 <= percent && percent <= 1);
    return static_cast<T>(a * (1 - percent) + b * percent);
}

inline color_t Blend(color_t a, color_t b, float percent)
{
    uint32_t alpha = Lerp((a >> 24) & 0xFF, (b >> 24) & 0xFF, percent) & 0xFF;
    uint32_t red = Lerp((a >> 16) & 0xFF, (b >> 16) & 0xFF, percent) & 0xFF;
    uint32_t green = Lerp((a >> 8) & 0xFF, (b >> 8) & 0xFF, percent) & 0xFF;
    uint32_t blue = Lerp((a >> 0) & 0xFF, (b >> 0) & 0xFF, percent) & 0xFF;
    return (alpha << 24) | (red << 16) | (green << 8) | blue;
}