#pragma once

#include "RayTracing/Effects/IndirectDiffuse/IndirectDiffuseUniformData.h"

#include <cstdint>

struct IndirectDiffuseSettings;
struct RayTracingPassCapabilities;

namespace IndirectDiffusePassData
{
	IndirectDiffuseUniformData Build(
	    const IndirectDiffuseSettings& settings,
	    const RayTracingPassCapabilities& capabilities,
	    std::uint32_t hitInstanceCount,
	    std::uint32_t hitMaterialCount,
	    std::uint32_t materialTextureTableCapacity) noexcept;
}
