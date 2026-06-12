#pragma once

#include "RHI/Public/Interop/RhiNativeHandles.h"
#include "RHI/Public/Resources/RhiResourceDesc.h"

#include <cstdint>

struct RayTracingSceneFrameData
{
	bool IsAvailable = false;
	RhiOwnedResourceHandle TlasResource = {};
	RhiGpuVirtualAddress TlasGpuAddress = 0;
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
