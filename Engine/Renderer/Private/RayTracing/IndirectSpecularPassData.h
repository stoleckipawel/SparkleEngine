#pragma once

#include "RayTracing/IndirectSpecularUniformData.h"

struct IndirectSpecularSettings;

namespace IndirectSpecularPassData
{
	IndirectSpecularUniformData Build(
	    const IndirectSpecularSettings& settings,
	    bool hitDataAvailable,
	    std::uint32_t hitInstanceCount,
	    std::uint32_t hitMaterialCount,
	    bool materialTextureTableAvailable,
	    std::uint32_t materialTextureTableDescriptorCount,
	    std::uint32_t materialTextureTableCapacity) noexcept;
}
