#pragma once

#include "RHI/Public/Interop/RhiNativeHandles.h"
#include "RHI/Public/Resources/RhiResourceDesc.h"

#include <cstdint>

struct RayTracingSceneFrameData
{
	bool IsAvailable = false;
	NativeResourceHandle TlasResource = {};
	RhiGpuVirtualAddress TlasGpuAddress = 0;
	std::uint32_t EstimatedInstanceCount = 0;

	bool HasBoundTlas() const noexcept
	{
		return IsAvailable && TlasResource && TlasGpuAddress != 0;
	}
};
