#pragma once
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#include "glm/glm.hpp"
#include "Debug/Debug.h"
#include <type_traits>
#include <tuple>
#include "Data/Serialization/Serialization.h"

class Tile;
class TileNeighbors;

template<typename T>
class THandle;

typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned int uint;
typedef uint16_t uint16;

static constexpr int CHUNK_SIZE_X = 8;
static constexpr int CHUNK_SIZE_Y = 8;
static constexpr int CHUNK_SIZE_Z = 1;

static constexpr uint LOCATION_MAX_X = 1 << 12;
static constexpr uint LOCATION_MAX_Y = 1 << 12;
static constexpr uint LOCATION_MAX_Z = 1 << 7;

static constexpr uint CHUNK_MAX_X = LOCATION_MAX_X / CHUNK_SIZE_X;
static constexpr uint CHUNK_MAX_Y = LOCATION_MAX_Y / CHUNK_SIZE_Y;
static constexpr uint CHUNK_MAX_Z = LOCATION_MAX_Z / CHUNK_SIZE_Z;

#ifdef _DEBUG
#define LINK_TILE
#endif

template<typename T>
T ModulusNegative(T value, T modulus)
{
    ASSERT(modulus > 0);
    return ((value % modulus) + modulus) % modulus;
}

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

    bool operator==(const Vec2& rhs)
    {
        return this->x == rhs.x && this->y == rhs.y;
    }
};

struct Vec3
{
    Vec3() : x(0), y(0), z(0) {}
    Vec3(short inX, short inY, short inZ) : x(inX), y(inY), z(inZ) {}
    Vec3(int inX, int inY, int inZ) : x((short)inX), y((short)inY), z((short)inZ) {}

    static Vec3 WrapPosition(Vec3 inPosition);
    static Vec3 WrapChunk(Vec3 inChunk);

    Vec3& operator+= (const Vec3& rhs) // compound assignment (does not need to be a member,
    {
        x += rhs.x;
        y += rhs.y;
        z += rhs.z;
        return *this; // return the result by reference
    }

    // friends defined inside class body are inline and are hidden from non-ADL lookup
    friend Vec3 operator+(Vec3 lhs, const Vec3 & rhs)
    {
        lhs += rhs; // reuse compound assignment
        return lhs; // return the result by value (uses move constructor)
    }

    Vec3& operator*= (const Vec3& rhs) // compound assignment (does not need to be a member,
    {
        x *= rhs.x;
        y *= rhs.y;
        z *= rhs.z;
        return *this; // return the result by reference
    }

    // friends defined inside class body are inline and are hidden from non-ADL lookup
    friend Vec3 operator*(Vec3 lhs, const Vec3& rhs)
    {
        lhs *= rhs; // reuse compound assignment
        return lhs; // return the result by value (uses move constructor)
    }

    friend bool operator==(const Vec3& lhs, const Vec3& rhs)
    {
        return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z;
    }

    friend bool operator<(const Vec3& lhs, const Vec3& rhs)
    {
        return std::tie(lhs.x, lhs.y, lhs.z) < std::tie(rhs.x, rhs.y, rhs.z);
    }

    short x;
    short y;
    short z;
};

namespace std {

    inline size_t HashPtr(const char* ptr, size_t size)
    {
        size_t value = 0;
        for (const char* it = ptr; it < (ptr + size); it++)
        {
            value = (hash<char>()(*it)) ^ (value << 1);
        }

        return value;
    }

    template <> struct hash<Vec3>
    {
        size_t operator()(const Vec3& value) const
        {
            const char* asChar = (char*)(&value);
            return HashPtr(asChar, sizeof(Vec3));
        }
    };
}

enum Direction : char
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

inline static Vec2 Vector2FromDirection(const Direction direction)
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

inline static Vec3 VectorFromDirection(const Direction direction)
{
    switch (direction)
    {
    case North:
        return Vec3(0, 1, 0);
    case NorthEast:
        return Vec3(1, 1, 0);
    case East:
        return Vec3(1, 0, 0);
    case SouthEast:
        return Vec3(1, -1, 0);
    case South:
        return Vec3(0, -1, 0);
    case SouthWest:
        return Vec3(-1, -1, 0);
    case West:
        return Vec3(-1, 0, 0);
    case NorthWest:
        return Vec3(-1, 1, 0);
    }
}

inline static Direction Rotate(Direction direction, Direction rotation)
{
    return (Direction) ((direction + rotation) % 8);
}

inline static Direction FindRotationBetween(Direction direction, Direction newDir)
{
    return (Direction)(((newDir - direction) + 8) % 8);
}

inline static Direction ReverseRotation(Direction rotation)
{
    return (Direction)((8 - rotation) % 8);
}

class Location
{
public:
    Location();
    Location(ushort x, ushort y, ushort z);
    Location(Vec3 vector) : Location(vector.x, vector.y, vector.z) {};

    bool GetValid() const;
    void SetValid(bool valid);
    ushort x() const;
    ushort y() const;
    ushort z() const;

    Vec3 GetVector() const;
    void SetVector(Vec3 vector);

    Vec2 AsVec2();
    Vec3 GetChunkPosition();
    Vec3 GetChunkLocalPosition();


    Tile& GetTile();
    Tile* operator ->();

    Location GetNeighbor(Direction direction);

    friend bool operator<(const Location& l, const Location& r)
    {
#ifndef _DEBUG
        return l.m_data < r.m_data;
#else
        return std::tie(l.m_valid, l.m_x, l.m_y, l.m_z) <
            std::tie(r.m_valid, r.m_x, r.m_y, r.m_z);
#endif
    }

#ifndef _DEBUG
    const int& GetData() const { return m_data; }
    void SetData(int newData) { m_data = newData; }
#endif // _DEBUG

    std::pair<Location, Direction> Traverse(Vec2 offset, Direction rotation = North);
    std::pair<Location, Direction> Traverse(short xOffset, short yOffset, Direction rotation = North);
    std::pair<Location, Direction> Traverse(Direction direction, Direction rotation = North);

    std::pair<Location, Direction> _Traverse_No_Neighbor(Direction direction);
    std::pair<Location, Direction> _Traverse_Neighbors(Direction direction);

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

inline Vec2 operator/(Vec2 lhs, int rhs)
{
    return Vec2(lhs.x / rhs, lhs.y / rhs);
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

union Color
{
    struct {
        uchar r;
        uchar g;
        uchar b;
        uchar a;
    };
    uint32_t color;

    Color() : r(255), g(0), b(255), a(255) {}

    Color(uchar _r, uchar _g, uchar _b)
    {
        r = _r;
        g = _g;
        b = _b;
        a = 255;
    }

    Color(uchar _r, uchar _g, uchar _b, uchar _a)
    {
        r = _r;
        g = _g;
        b = _b;
        a = _a;
    }

    bool operator==(const Color& other) const { return color == other.color; }
    operator glm::vec4() const { return glm::vec4(((float)r) / 255, ((float)g) / 255, ((float)b) / 255, ((float)a) / 255); }
};

template<typename T>
T Lerp(T a, T b, float percent)
{
    ASSERT(0 <= percent && percent <= 1);
    return static_cast<T>(a * (1 - percent) + b * percent);
}

template<typename T>
T Clamp(T value, T min, T max)
{
    ASSERT(min <= max);
    if (value < min) { value = min; }
    if (value > max) value = max;
    return value;
}

inline float Clamp(float value)
{
    return Clamp(value, 0.0f, 1.0f);
}

inline Color Blend(Color f, Color s, float percent)
{
    uchar r = Lerp(f.r, s.r, percent);
    uchar g = Lerp(f.g, s.g, percent);
    uchar b = Lerp(f.b, s.b, percent);
    uchar a = Lerp(f.a, s.a, percent);
    return Color(r, g, b, a);
}

namespace Serialization
{
    template<typename Stream>
    void SerializeObject(Stream& stream, const Direction& direction)
    {
        int asInt = direction;
        Serialize(stream, asInt);
    }

    template<typename Stream>
    void DeserializeObject(Stream& stream, Direction& direction)
    {
        int asInt;
        Deserialize(stream, asInt);
        direction = (Direction) asInt;
    }

    template<typename Stream>
    void Serialize(Stream& stream, const Vec2& value)
    {
        Write(stream, "x", value.x);
        Write(stream, "y", value.y);
    }

    template<typename Stream>
    void Deserialize(Stream& stream, Vec2& value)
    {
        Read(stream, "x", value.x);
        Read(stream, "y", value.y);
    }

    template<typename Stream>
    void Serialize(Stream& stream, const Vec3& value)
    {
        Write(stream, "x", value.x);
        Write(stream, "y", value.y);
        Write(stream, "z", value.z);
    }

    template<typename Stream>
    void Deserialize(Stream& stream, Vec3& value)
    {
        Read(stream, "x", value.x);
        Read(stream, "y", value.y);
        Read(stream, "z", value.z);
    }

    template<typename Stream>
    void Serialize(Stream& stream, const Location& value)
    {
#ifdef _DEBUG
        bool valid = value.GetValid();
        Write(stream, "Valid", valid);
        if (value.GetValid())
        {
            Vec3 vec = value.GetVector();
            Write(stream, "Vector", vec);
        }
#else
        Write(stream, "Data", value.GetData());
#endif
    }

    template<typename Stream>
    void Deserialize(Stream& stream, Location& value)
    {
#ifdef _DEBUG
        value.SetValid(Read<Stream, bool>(stream, "Valid"));
        if (value.GetValid())
        {
            value.SetVector(Read<Stream, Vec3>(stream, "Vector"));
        }
#else
        value.SetData(Read<Stream, int>(stream, "Data"));
#endif
    }

    template<typename Stream>
    void Serialize(Stream& stream, const Color& value)
    {
        if constexpr (std::is_same<Stream, JSONStream>::value)
        {
            Write(stream, "r", value.r);
            Write(stream, "g", value.g);
            Write(stream, "b", value.b);
            Write(stream, "a", value.a);
        }
        else
        {
            Write(stream, "color", value.color);
        }
    }

    template<typename Stream>
    void Deserialize(Stream& stream, Color& value)
    {
        if constexpr (std::is_same<Stream, JSONStream>::value)
        {
            Read(stream, "r", value.r);
            Read(stream, "g", value.g);
            Read(stream, "b", value.b);
            Read(stream, "a", value.a);
        }
        else
        {
            Read(stream, "color", value.color);
        }
    }
}