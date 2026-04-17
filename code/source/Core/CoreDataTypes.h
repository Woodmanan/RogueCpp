#pragma once
#include "CoreDataTypes.Forward.h"
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#include "glm/glm.hpp"
#include "Debug/Debug.h"
#include <type_traits>
#include <tuple>
#include "Data/Serialization/Serialization.h"
#include "Data/Serialization/BitStream.h"

#ifdef _DEBUG
#define LINK_TILE
#endif

template<typename T>
T ModulusNegative(T value, T modulus)
{
    ASSERT(modulus > 0);

	if (value < 0)
    {
        value = modulus - (-value % modulus);
    }

    T out = value % modulus;

	ASSERT(out >= 0 && out < modulus);
    return out;
}

struct Vec2;
struct Vec3;
struct Vec4;

struct Vec2
{
	Vec2() = default;
	Vec2(int X, int Y) : x(X), y(Y) {}

	int x = 0;
	int y = 0;

    Vec2& operator+=(const Vec2& rhs) 
    {
        x += rhs.x;
        y += rhs.y;
        return *this;
    }

    friend Vec2 operator+(Vec2 lhs, const Vec2& rhs)
    {
        lhs += rhs;
        return lhs;
    }

	Vec2& operator-=(const Vec2& rhs) 
    {
        x -= rhs.x;
        y -= rhs.y;
        return *this;
    }

    friend Vec2 operator-(Vec2 lhs, const Vec2& rhs)
    {
        lhs -= rhs;
        return lhs;
    }

    Vec2& operator*= (const Vec2& rhs)
    {
        x *= rhs.x;
        y *= rhs.y;
        return *this;
    }

    friend Vec2 operator*(Vec2 lhs, const Vec2& rhs)
    {
        lhs *= rhs;
        return lhs;
    } 

	Vec2& operator*= (const int& rhs)
    {
        x *= rhs;
        y *= rhs;
        return *this;
    }

    friend Vec2 operator*(Vec2 lhs, const int& rhs)
    {
        lhs *= rhs;
        return lhs;
    }

	Vec2& operator/= (const int& rhs)
    {
        x /= rhs;
        y /= rhs;
        return *this;
    }

    friend Vec2 operator/(Vec2 lhs, const int& rhs)
    {
        lhs /= rhs;
        return lhs;
    }

    bool operator==(const Vec2& rhs) const
    {
        return x == rhs.x && y == rhs.y;
    }

	friend bool operator<(const Vec2& lhs, const Vec2& rhs)
    {
        return std::tie(lhs.x, lhs.y) < std::tie(rhs.x, rhs.y);
    }
};

struct Vec3 : Vec2
{
	Vec3() = default;
	Vec3(int X, int Y, int Z) : Vec2(X, Y), z(Z) {}

	//Conversion
	Vec3(const Vec2& Vec) : Vec2(Vec), z(0) {}
	operator Vec2() const { return Vec2(x, y); }
	

	int z = 0;

	Vec3& operator+=(const Vec3& rhs) 
    {
        x += rhs.x;
        y += rhs.y;
		z += rhs.z;
        return *this;
    }

    friend Vec3 operator+(Vec3 lhs, const Vec3& rhs)
    {
        lhs += rhs;
        return lhs;
    }

	Vec3& operator-=(const Vec3& rhs) 
    {
        x -= rhs.x;
        y -= rhs.y;
		z -= rhs.z;
        return *this;
    }

    friend Vec3 operator-(Vec3 lhs, const Vec3& rhs)
    {
        lhs -= rhs;
        return lhs;
    }

    Vec3& operator*= (const Vec3& rhs)
    {
        x *= rhs.x;
        y *= rhs.y;
		z *= rhs.z;
        return *this;
    }

    friend Vec3 operator*(Vec3 lhs, const Vec3& rhs)
    {
        lhs *= rhs;
        return lhs;
    } 

	Vec3& operator*= (const int& rhs)
    {
        x *= rhs;
        y *= rhs;
        z *= rhs;
        return *this;
    }

    friend Vec3 operator*(Vec3 lhs, const int& rhs)
    {
        lhs *= rhs;
        return lhs;
    }

	Vec3& operator/= (const int& rhs)
    {
        x /= rhs;
        y /= rhs;
        z /= rhs;
        return *this;
    }

    friend Vec3 operator/(Vec3 lhs, const int& rhs)
    {
        lhs /= rhs;
        return lhs;
    }

    bool operator==(const Vec3& rhs) const
    {
        return x == rhs.x && y == rhs.y && z == rhs.z;
    }

	friend bool operator<(const Vec3& lhs, const Vec3& rhs)
    {
        return std::tie(lhs.x, lhs.y, lhs.z) < std::tie(rhs.x, rhs.y, rhs.z);
    }
};

struct Vec4 : Vec3
{
	Vec4() = default;
	Vec4(int X, int Y, int Z, int W) : Vec3(X, Y, Z), w(W) {}
	Vec4(int X, int Y) : Vec4(X, Y, 0, 0) {}
	Vec4(int X, int Y, int Z) : Vec4(X, Y, Z, 0) {}

	//Conversion
	Vec4(const Vec2& Vec) : Vec3(Vec), w(0) {}
	Vec4(const Vec3& Vec) : Vec3(Vec), w(0) {}
	operator Vec2() const { return Vec2(x, y); }
	operator Vec3() const { return Vec3(x, y, z); }

	int w = 0;

	Vec4& operator+=(const Vec4& rhs) 
    {
        x += rhs.x;
        y += rhs.y;
		z += rhs.z;
		w += rhs.w;
        return *this;
    }

    friend Vec4 operator+(Vec4 lhs, const Vec4& rhs)
    {
        lhs += rhs;
        return lhs;
    }

	Vec4& operator-=(const Vec4& rhs) 
    {
        x -= rhs.x;
        y -= rhs.y;
		z -= rhs.z;
		w -= rhs.w;
        return *this;
    }

    friend Vec4 operator-(Vec4 lhs, const Vec4& rhs)
    {
        lhs -= rhs;
        return lhs;
    }

    Vec4& operator*= (const Vec4& rhs)
    {
        x *= rhs.x;
        y *= rhs.y;
		z *= rhs.z;
		w *= rhs.w;
        return *this;
    }

    friend Vec4 operator*(Vec4 lhs, const Vec4& rhs)
    {
        lhs *= rhs;
        return lhs;
    } 

	Vec4 operator/= (const Vec4& rhs)
	{
		x /= rhs.x;
		y /= rhs.y;
		z /= rhs.z;
		w /= rhs.w;
		return *this;
	}

	friend Vec4 operator/(Vec4 lhs, const Vec4& rhs)
    {
        lhs /= rhs;
        return lhs;
    }

	Vec4 operator%= (const Vec4& rhs)
	{
		x %= rhs.x;
		y %= rhs.y;
		z %= rhs.z;
		w %= rhs.w;
		return *this;
	}

	friend Vec4 operator%(Vec4 lhs, const Vec4& rhs)
    {
        lhs %= rhs;
        return lhs;
    }

	Vec4& operator*= (const int& rhs)
    {
        x *= rhs;
        y *= rhs;
        z *= rhs;
        w *= rhs;
        return *this;
    }

    friend Vec4 operator*(Vec4 lhs, const int& rhs)
    {
        lhs *= rhs;
        return lhs;
    }

	Vec3& operator/= (const int& rhs)
    {
        x /= rhs;
        y /= rhs;
        z /= rhs;
        w /= rhs;
        return *this;
    }

    friend Vec4 operator/(Vec4 lhs, const int& rhs)
    {
        lhs /= rhs;
        return lhs;
    }

    bool operator==(const Vec4& rhs) const
    {
        return x == rhs.x && y == rhs.y && z == rhs.z && w == rhs.w;
    }

	friend bool operator<(const Vec4& lhs, const Vec4& rhs)
    {
        return std::tie(lhs.x, lhs.y, lhs.z, lhs.w) < std::tie(rhs.x, rhs.y, rhs.z, lhs.w);
    }

	static Vec4 WrapPosition(Vec4 inPosition);
	static Vec4 WrapChunk(Vec4 inChunk);
};

struct Rect3
{
    Vec3 m_lower;
    Vec3 m_upper;
};

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
    Location(uint x, uint y, uint z = 0, uint w = 0);
    Location(Vec4 vector) : Location(vector.x, vector.y, vector.z, vector.w) {};

    bool GetValid() const;
    void SetValid(bool valid);
    uint x() const;
    uint y() const;
    uint z() const;
    uint w() const;

	Vec4 GetVector() const;
    void SetVector(Vec4 vector);


    Vec2 AsVec2();
    Vec4 GetChunkPosition();
    Vec4 GetChunkLocalPosition();


    Tile& GetTile();
    Tile* operator ->();

    Location GetNeighbor(Direction direction);

    friend bool operator<(const Location& l, const Location& r)
    {
		bool lValid = l.GetValid();
		bool rValid = r.GetValid();
        return std::tie(lValid, l.m_vec) <
               std::tie(rValid, r.m_vec);
    }

    friend bool operator==(const Location& l, const Location& r)
    {
		if (!l.GetValid() && !r.GetValid())
		{
			return true;
		}

		return l.m_vec == r.m_vec;
    }

    std::pair<Location, Direction> Traverse(Vec2 offset, Direction rotation = North);
    std::pair<Location, Direction> Traverse(short xOffset, short yOffset, Direction rotation = North);
    std::pair<Location, Direction> Traverse(Direction direction, Direction rotation = North);

    std::pair<Location, Direction> _Traverse_No_Neighbor(Direction direction);
    std::pair<Location, Direction> _Traverse_Neighbors(Direction direction);

    THandle<TileNeighbors> GetNeighbors();

    static const uint invalidMask = 0x80000000;

private:
	Vec4 m_vec;

#ifdef LINK_TILE
private:
    Tile* linkedTile = nullptr;
    void RefreshLinkedTile();
#endif

	friend class Serialization::Serializer<Location>;
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

namespace std
{
    inline size_t HashPtr(const char* ptr, size_t size)
    {
        size_t value = 0;
        for (const char* it = ptr; it < (ptr + size); it++)
        {
            value = (hash<char>()(*it)) ^ (value << 1);
        }

        return value;
    }

    template <> struct hash<Vec2>
    {
        size_t operator()(const Vec2& value) const
        {
            const char* asChar = (char*)(&value);
            return HashPtr(asChar, sizeof(Vec2));
        }
    };

    template <> struct hash<Vec3>
    {
        size_t operator()(const Vec3& value) const
        {
            const char* asChar = (char*)(&value);
            return HashPtr(asChar, sizeof(Vec3));
        }
    };

    template <> struct hash<Vec4>
    {
        size_t operator()(const Vec4& value) const
        {
            const char* asChar = (char*)(&value);
            return HashPtr(asChar, sizeof(Vec4));
        }
    };

    template <> struct hash<Location>
    {
        size_t operator()(const Location& value) const
        {
            const char* asChar = (char*)(&value);
            return HashPtr(asChar, sizeof(Location));
        }
    };
}

namespace Serialization
{
    template<>
    struct Serializer<Direction> : EnumSerializer<Direction> {};

    template<>
    struct Serializer<Vec2> : ObjectSerializer<Vec2>
    {
		template<typename Stream>
    	static void Serialize(Stream& stream, const Vec2& value)
		{
			Write(stream, "x", value.x);
			Write(stream, "y", value.y);
		}

    	template<typename Stream>
    	static void Deserialize(Stream& stream, Vec2& value)
		{
			Read(stream, "x", value.x);
			Read(stream, "y", value.y);
		}
    };

    template<>
    struct Serializer<Vec3> : ObjectSerializer<Vec3>
    {   
		template<typename Stream>
    	static void Serialize(Stream& stream, const Vec3& value)
		{
			Write(stream, "x", value.x);
			Write(stream, "y", value.y);
			Write(stream, "z", value.z);
		}

    	template<typename Stream>
    	static void Deserialize(Stream& stream, Vec3& value)
		{
			Read(stream, "x", value.x);
			Read(stream, "y", value.y);
			Read(stream, "z", value.z);
		}
    };

	template<>
    struct Serializer<Vec4> : ObjectSerializer<Vec4>
    {   
		template<typename Stream>
    	static void Serialize(Stream& stream, const Vec4& value)
		{
			Write(stream, "x", value.x);
			Write(stream, "y", value.y);
			Write(stream, "z", value.z);
			Write(stream, "w", value.w);
		}

    	template<typename Stream>
    	static void Deserialize(Stream& stream, Vec4& value)
		{
			Read(stream, "x", value.x);
			Read(stream, "y", value.y);
			Read(stream, "z", value.z);
			Read(stream, "w", value.w);
		}
    };
    
    template<>
    struct Serializer<Location> : ObjectSerializer<Location>
    {
    	template<typename Stream>
    	static void Serialize(Stream& stream, const Location& value)
		{
			bool valid = value.GetValid();
			Write(stream, "Valid", valid);
			if (value.GetValid())
			{
				Write(stream, "Vector", value.m_vec);
			}
		}

    	template<typename Stream>
    	static void Deserialize(Stream& stream, Location& value)
    	{
            value.SetValid(Read<Stream, bool>(stream, "Valid"));
            if (value.GetValid())
            {
				Read(stream, "Vector", value.m_vec);
            }
    	}
    };


    template<>
    struct Serializer<Color> : ObjectSerializer<Color>
    {

    	template<typename Stream>
    	static void Serialize(Stream& stream, const Color& value)
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
    	static void Deserialize(Stream& stream, Color& value)
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
    };
}
