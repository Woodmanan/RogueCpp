#pragma once
#include "Debug/Debug.h"

using uint32_t = uint;

constexpr int RoundUpPowerOfTwo(int value)
{
	value--;
	value |= value >>  1;
	value |= value >>  2;
	value |= value >>  4;
	value |= value >>  8;
	value |= value >> 16;
	return ++value;
}

constexpr uint RoundUpPowerOfTwo(uint value)
{
	value--;
	value |= value >>  1;
	value |= value >>  2;
	value |= value >>  4;
	value |= value >>  8;
	value |= value >> 16;
	return ++value;
}

constexpr uint RoundUpNearestMultiple(uint value, uint multiple)
{
	return (((value - 1) / multiple) + 1) * multiple;
}