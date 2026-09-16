#pragma once

#include "RHI/Public/Shaders/ShaderMap.h"

#include <cstdint>

struct RayTracingMaterialPayload final
{
	float RayT = 0.0f;
	std::uint32_t InstanceId = 0u;
	std::uint32_t PrimitiveIndex = 0u;
	float Barycentrics[2] = {};
	std::uint32_t Hit = 0u;
	std::uint32_t FrontFace = 0u;
};

inline constexpr RayTracingShaderMetadata kRayTracingMaterialShaderMetadata =
    BuildRayTracingShaderMetadata<RayTracingMaterialPayload, RayTracingTriangleAttributeLayout>(1u);
