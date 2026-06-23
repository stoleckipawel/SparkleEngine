#pragma once

#include "Core/Public/Console/CVar.h"
#include "RayTracing/Effects/IndirectDiffuse/IndirectDiffuseDebugMode.h"

extern ConsoleVariable<bool> CVarIndirectDiffuseEnabled;
extern ConsoleVariable<IndirectDiffuseDebugMode> CVarIndirectDiffuseDebugMode;
extern ConsoleVariable<float> CVarIndirectDiffuseNormalBias;
extern ConsoleVariable<float> CVarIndirectDiffuseMaxDistance;
extern ConsoleVariable<float> CVarIndirectDiffuseIntensity;
