#pragma once

#include "Core/Public/Console/CVar.h"
#include "RayTracing/RTIndirectSpecularDebugMode.h"
#include "RayTracing/RTIndirectSpecularSampleMode.h"

extern ConsoleVariable<bool> CVarRTIndirectSpecularEnabled;
extern ConsoleVariable<RTIndirectSpecularDebugMode> CVarRTIndirectSpecularDebugMode;
extern ConsoleVariable<RTIndirectSpecularSampleMode> CVarRTIndirectSpecularSampleMode;
extern ConsoleVariable<float> CVarRTIndirectSpecularNormalBias;
extern ConsoleVariable<float> CVarRTIndirectSpecularMaxDistance;
