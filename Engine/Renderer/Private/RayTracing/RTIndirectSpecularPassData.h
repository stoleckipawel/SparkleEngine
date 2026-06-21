#pragma once

#include "RayTracing/RTIndirectSpecularUniformData.h"

struct RTIndirectSpecularSettings;

namespace RTIndirectSpecularPassData
{
	RTIndirectSpecularUniformData Build(
	    const RTIndirectSpecularSettings& settings,
	    bool hitDataAvailable,
	    std::uint32_t hitInstanceCount,
	    std::uint32_t hitMaterialCount,
	    bool materialTextureTableAvailable,
	    std::uint32_t materialTextureTableDescriptorCount,
	    std::uint32_t materialTextureTableCapacity) noexcept;
}
