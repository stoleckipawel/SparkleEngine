#pragma once

#include "Core/Public/Console/CVar.h"

#include <cstdint>

extern ConsoleVariable<bool> CVarReferencePathTracingEnabled;
extern ConsoleVariable<std::uint32_t> CVarReferencePathTracingSamplesPerPixel;
extern ConsoleVariable<std::uint32_t> CVarReferencePathTracingBounceCount;
extern ConsoleVariable<float> CVarReferencePathTracingNormalBias;
extern ConsoleVariable<float> CVarReferencePathTracingMaxDistance;
