#pragma once

#include "Core/Public/Console/CVar.h"

#include <cstdint>

extern ConsoleVariable<std::uint32_t> CVarReferencePathTracerSamplesPerPixel;
extern ConsoleVariable<std::uint32_t> CVarReferencePathTracerBounceCount;
extern ConsoleVariable<float> CVarReferencePathTracerNormalBias;
extern ConsoleVariable<float> CVarReferencePathTracerMaxDistance;
