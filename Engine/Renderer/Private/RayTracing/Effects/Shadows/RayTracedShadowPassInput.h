#pragma once

#include "RayTracing/Effects/Shadows/RayTracedShadowSettings.h"
#include "RHI/Public/Resources/RhiResourceDesc.h"
#include "RHI/Public/Resources/RhiResourceHandles.h"

#include <cstdint>

struct RayTracedShadowPassInput final
{
	RayTracedShadowSettings Settings = {};
	RhiGpuVirtualAddress SceneTlasGpuAddress = 0u;
	std::uint32_t HitInstanceCount = 0u;
	std::uint32_t HitMaterialCount = 0u;
	bool Enabled = false;
};
