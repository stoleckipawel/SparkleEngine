#pragma once

#include <cstdint>

enum class RayTracingSceneTlasShaderAccessMode : std::uint32_t
{
	Descriptor = 0u,
	ShaderDeviceAddress = 1u,
};
