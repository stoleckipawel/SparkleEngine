#pragma once

#include "RHI/Public/Resources/RhiResourceHandles.h"

#include <cstdint>

struct RenderRayTracingFrameBindings final
{
	RhiOwnedResourceHandle TlasResource = {};
	std::uint32_t EstimatedInstanceCount = 0;

	bool HasBoundTlas() const noexcept { return static_cast<bool>(TlasResource); }

	bool HasTraceableInstances() const noexcept { return HasBoundTlas() && EstimatedInstanceCount > 0; }
};
