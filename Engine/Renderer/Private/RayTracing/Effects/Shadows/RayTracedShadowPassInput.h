#pragma once

#include "RayTracing/Effects/Shadows/RayTracedShadowSettings.h"
#include "RHI/Public/Resources/RhiResourceHandles.h"

struct RayTracedShadowPassInput final
{
	RayTracedShadowSettings Settings = {};
	RhiGpuVirtualAddress SceneTlasGpuAddress = 0u;
	bool Enabled = false;
};
