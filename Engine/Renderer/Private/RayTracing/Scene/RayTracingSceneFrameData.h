#pragma once

#include "RayTracing/Scene/RayTracingSceneTlasShaderAccessMode.h"
#include "RHI/Public/Resources/RhiResourceHandles.h"
#include "RHI/Public/Resources/RhiResourceDesc.h"

#include <cstdint>

struct RayTracingSceneFrameData final
{
	bool IsAvailable = false;
	RhiOwnedResourceHandle TlasResource = {};
	RhiGpuVirtualAddress TlasGpuAddress = 0;
	RayTracingSceneTlasShaderAccessMode TlasShaderAccessMode = RayTracingSceneTlasShaderAccessMode::Descriptor;
	std::uint32_t EstimatedInstanceCount = 0;

	bool HasBoundTlas() const noexcept
	{
		return IsAvailable && TlasResource && TlasGpuAddress != 0;
	}

	bool HasTraceableInstances() const noexcept
	{
		return HasBoundTlas() && EstimatedInstanceCount > 0;
	}
};
