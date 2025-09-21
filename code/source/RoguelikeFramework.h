// RoguelikeFramework.h : Include file for standard system include files,
// or project specific include files.

#pragma once

#include <iostream>
#include "Data/Serialization/Serialization.h"

enum class EDrawState
{
	Normal,
	Temperature,
	COUNT
};

void CycleDrawState(EDrawState& state)
{
	state = (EDrawState)(((int)state + 1) % ((int)EDrawState::COUNT));
}

void SetupResources();
// TODO: Reference additional headers your program requires here.