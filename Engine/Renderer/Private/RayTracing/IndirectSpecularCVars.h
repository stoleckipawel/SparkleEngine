#pragma once

#include "Core/Public/Console/CVar.h"
#include "RayTracing/IndirectSpecularDebugMode.h"
#include "RayTracing/IndirectSpecularSampleMode.h"

extern ConsoleVariable<bool> CVarIndirectSpecularEnabled;
extern ConsoleVariable<IndirectSpecularDebugMode> CVarIndirectSpecularDebugMode;
extern ConsoleVariable<IndirectSpecularSampleMode> CVarIndirectSpecularSampleMode;
extern ConsoleVariable<float> CVarIndirectSpecularNormalBias;
extern ConsoleVariable<float> CVarIndirectSpecularMaxDistance;
