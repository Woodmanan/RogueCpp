#pragma once

class Tile;
class TileNeighbors;
class TileStats;

template<typename T>
class THandle;

typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned int uint;
typedef unsigned long ulong;
constexpr uchar MAX_UCHAR   = 0xFF;
constexpr ushort MAX_USHORT = 0xFFFF;
constexpr uint MAX_UINT     = 0xFFFFFFFF;
constexpr ulong MAX_ULONG   = 0xFFFFFFFFFFFFFFFF;

static constexpr int CHUNK_SIZE_X = 8;
static constexpr int CHUNK_SIZE_Y = 8;
static constexpr int CHUNK_SIZE_Z = 1;
static constexpr int CHUNK_SIZE_W = 1;

static constexpr int LOCATION_MAX_X = 0x7FFFFFFF;
static constexpr int LOCATION_MAX_Y = 0x7FFFFFFF;
static constexpr int LOCATION_MAX_Z = 0x7FFFFFFF;
static constexpr int LOCATION_MAX_W = 0x7FFFFFFF;

static constexpr int CHUNK_MAX_X = LOCATION_MAX_X / CHUNK_SIZE_X;
static constexpr int CHUNK_MAX_Y = LOCATION_MAX_Y / CHUNK_SIZE_Y;
static constexpr int CHUNK_MAX_Z = LOCATION_MAX_Z / CHUNK_SIZE_Z;
static constexpr int CHUNK_MAX_W = LOCATION_MAX_W / CHUNK_SIZE_W;

static constexpr float BIG_FLOAT = 1000000000.0f;
