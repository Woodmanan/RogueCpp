#pragma once

#define UNUSED(...) do { __noop(__VA_ARGS__); } while(false)

#ifdef _DEBUG
#define BREAK() __debugbreak()
#define HALT() do {BREAK(); exit(EXIT_FAILURE);} while(false)
#define ASSERT(x) (!(x)) ? BREAK() : (void)0
#else
#define BREAK() do;while(false)
#define HALT() do {BREAK(); exit(EXIT_FAILURE);} while(false)
#define ASSERT(x) UNUSED(x)
#endif // DEBUG

#define STRONG_ASSERT(x) do {if (!(x)){HALT();}} while(false)

