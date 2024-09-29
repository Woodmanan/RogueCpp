#pragma once
#include "tracy/Tracy.hpp"
#include "Debug.h"

#if (DEBUG || REL_WITH_DEBUG) && TRACY_ENABLE
#define USE_TRACY_PROFILING
#endif

#ifdef USE_TRACY_PROFILING
#define ROGUE_PROFILE ZoneScoped
#define ROGUE_PROFILE_FRAME() FrameMark
#define ROGUE_PROFILE_SECTION(x) ZoneScopedN(x)
#define ROGUE_PROFILE_TAG(y, x) ZoneText(x, strlen(x))
#define ROGUE_PROFILE_LOG(text, size) TracyMessage(text, size)
#define ROGUE_PROFILE_VALUE(text, value) TracyPlot(text, value)
#else
#define ROGUE_PROFILE UNUSED()
#define ROGUE_PROFILE_FRAME() UNUSED()
#define ROGUE_PROFILE_SECTION(x) UNUSED(x)
#define ROGUE_PROFILE_TAG(y, x) UNUSED(x, strlen(x))
#define ROGUE_PROFILE_LOG(text, size) UNUSED(text, size)
#define ROGUE_PROFILE_VALUE(text, value) UNUSED(text, value)
#endif