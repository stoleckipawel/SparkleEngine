#pragma once

#include "RayTracing/RTIndirectSpecularUniformData.h"

namespace RTIndirectSpecularPassData
{
	RTIndirectSpecularUniformData Build(bool hitDataAvailable, std::uint32_t hitInstanceCount, std::uint32_t hitMaterialCount) noexcept;
}
