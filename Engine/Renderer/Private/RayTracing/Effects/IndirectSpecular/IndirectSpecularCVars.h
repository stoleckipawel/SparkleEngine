#pragma once

#include "Core/Public/Console/CVar.h"
#include "RayTracing/Effects/IndirectSpecular/IndirectSpecularDebugMode.h"
#include "RayTracing/Effects/IndirectSpecular/IndirectSpecularSampleMode.h"

extern ConsoleVariable<bool> CVarIndirectSpecularEnabled;
extern ConsoleVariable<IndirectSpecularDebugMode> CVarIndirectSpecularDebugMode;
extern ConsoleVariable<IndirectSpecularSampleMode> CVarIndirectSpecularSampleMode;
extern ConsoleVariable<float> CVarIndirectSpecularNormalBias;
extern ConsoleVariable<float> CVarIndirectSpecularMaxDistance;
