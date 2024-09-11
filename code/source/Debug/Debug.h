#pragma once
#include "../Utils/Utils.h"

#define UNUSED(...) do { __noop(__VA_ARGS__); } while(false)

#ifdef _DEBUG
#define BREAK() __debugbreak()
#define HALT() do {BREAK(); exit(EXIT_FAILURE);} while(false)
#define ASSERT(x) (!(x)) ? BREAK() : (void)0
#define DEBUG_PRINT(x, ...) string_format_print(x, __VA_ARGS__)
#else
#define BREAK() do;while(false)
#define HALT() do {BREAK(); exit(EXIT_FAILURE);} while(false)
#define ASSERT(x) UNUSED(x)
#define DEBUG_PRINT(x, ...) UNUSED(x, __VA_ARGS__)
#endif // DEBUG

#define STRONG_ASSERT(x) do {if (!(x)){HALT();}} while(false)

