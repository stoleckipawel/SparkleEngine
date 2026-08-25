#pragma once

#include <cstdint>

struct RayTracingGBufferUniformData final
{
	std::uint32_t RayTracingHitInstanceCount = 0;
	std::uint32_t RayTracingHitMaterialCount = 0;
};
