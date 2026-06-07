#pragma once

#include "Core/Public/Console/CVar.h"
#include "RayTracing/RayTracedShadowDenoiserMode.h"
#include "RayTracing/RayTracedShadowQualityMode.h"

extern ConsoleVariable<RayTracedShadowQualityMode> CVarRayTracedShadowQualityMode;
extern ConsoleVariable<RayTracedShadowDenoiserMode> CVarRayTracedShadowDenoiserMode;
extern ConsoleVariable<float> CVarRayTracedShadowNormalBias;
extern ConsoleVariable<float> CVarRayTracedShadowMaxDistance;
