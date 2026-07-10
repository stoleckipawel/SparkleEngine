#pragma once

#include "Core/Public/Console/CVar.h"

#include <cstdint>

extern ConsoleVariable<std::uint32_t> CVarPathTracedLightingSamplesPerPixel;
extern ConsoleVariable<std::uint32_t> CVarPathTracedLightingBounceCount;
extern ConsoleVariable<float> CVarPathTracedLightingNormalBias;
extern ConsoleVariable<float> CVarPathTracedLightingMaxDistance;
