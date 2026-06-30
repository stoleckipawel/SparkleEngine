#pragma once

#include "Core/Public/Console/CVar.h"
#include "RayTracing/Effects/Shadows/RayTracedShadowDenoiserMode.h"

extern ConsoleVariable<RayTracedShadowDenoiserMode> CVarRayTracedShadowDenoiserMode;
extern ConsoleVariable<bool> CVarRayTracedShadowsEnabled;
extern ConsoleVariable<float> CVarRayTracedShadowNormalBias;
extern ConsoleVariable<float> CVarRayTracedShadowMaxDistance;
extern ConsoleVariable<bool> CVarRayTracedShadowDiagnosticsEnabled;
