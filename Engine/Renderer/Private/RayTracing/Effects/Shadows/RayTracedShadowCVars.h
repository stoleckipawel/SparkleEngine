#pragma once

#include "Core/Public/Console/CVar.h"
#include "RayTracing/Effects/Shadows/RayTracedShadowDenoiserMode.h"
#include "RayTracing/Effects/Shadows/RayTracedShadowQualityMode.h"

extern ConsoleVariable<RayTracedShadowQualityMode> CVarRayTracedShadowQualityMode;
extern ConsoleVariable<RayTracedShadowDenoiserMode> CVarRayTracedShadowDenoiserMode;
extern ConsoleVariable<bool> CVarRayTracedShadowsEnabled;
extern ConsoleVariable<float> CVarRayTracedShadowNormalBias;
extern ConsoleVariable<float> CVarRayTracedShadowMaxDistance;
extern ConsoleVariable<bool> CVarRayTracedShadowDiagnosticsEnabled;
