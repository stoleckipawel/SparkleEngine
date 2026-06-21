#pragma once

#include "RayTracing/RTIndirectSpecularUniformData.h"

struct RTIndirectSpecularSettings;

namespace RTIndirectSpecularPassData
{
	RTIndirectSpecularUniformData Build(
	    const RTIndirectSpecularSettings& settings,
	    bool hitDataAvailable,
	    std::uint32_t hitInstanceCount,
	    std::uint32_t hitMaterialCount) noexcept;
}
