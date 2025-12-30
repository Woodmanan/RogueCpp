#pragma once

class Tile;
class TileNeighbors;

template<typename T>
class THandle;

typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned int uint;

static constexpr int CHUNK_SIZE_X = 8;
static constexpr int CHUNK_SIZE_Y = 8;
static constexpr int CHUNK_SIZE_Z = 1;

static constexpr uint LOCATION_MAX_X = 1 << 12;
static constexpr uint LOCATION_MAX_Y = 1 << 12;
static constexpr uint LOCATION_MAX_Z = 1 << 7;

static constexpr uint CHUNK_MAX_X = LOCATION_MAX_X / CHUNK_SIZE_X;
static constexpr uint CHUNK_MAX_Y = LOCATION_MAX_Y / CHUNK_SIZE_Y;
static constexpr uint CHUNK_MAX_Z = LOCATION_MAX_Z / CHUNK_SIZE_Z;

static constexpr float BIG_FLOAT = 1000000000.0f;