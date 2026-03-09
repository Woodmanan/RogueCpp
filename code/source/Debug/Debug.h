#pragma once
#include "Utils/FormatUtils.h"

//#define UNUSED(...) do { __noop(__VA_ARGS__); } while(false)
#define UNUSED(...) do { (void)0; } while(0)


#if DEBUG
#define BREAK() __debugbreak()
#define ASSERT(x) (!(x)) ? BREAK() : (void)0
#define DEBUG_PRINT(x, ...) string_format_print(x __VA_OPT__(,)  __VA_ARGS__)

#else
#define BREAK() do;while(false)
#define ASSERT(x) UNUSED(x)
#define DEBUG_PRINT(x, ...) UNUSED(x __VA_OPT__(,) __VA_ARGS__)
#endif // DEBUG

#define HALT() do {BREAK(); exit(EXIT_FAILURE);} while(false)
#define STRONG_ASSERT(x) do {if (!(x)){HALT();}} while(false)
#define PRINT_ERR(x, ...) string_format_print_error(x __VA_OPT__(,) __VA_ARGS__)

