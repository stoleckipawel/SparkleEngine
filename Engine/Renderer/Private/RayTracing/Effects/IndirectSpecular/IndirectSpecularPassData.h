#pragma once

#include "RayTracing/Effects/IndirectSpecular/IndirectSpecularUniformData.h"

struct IndirectSpecularSettings;
struct RayTracingPassCapabilities;

namespace IndirectSpecularPassData
{
	IndirectSpecularUniformData Build(
	    const IndirectSpecularSettings& settings,
	    const RayTracingPassCapabilities& capabilities,
	    std::uint32_t hitInstanceCount,
	    std::uint32_t hitMaterialCount,
	    std::uint32_t materialTextureTableCapacity) noexcept;
}
