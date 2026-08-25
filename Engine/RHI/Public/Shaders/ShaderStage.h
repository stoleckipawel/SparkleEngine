#pragma once

#include "../RHIAPI.h"

#include <cstdint>
#include <string>

enum class ShaderStage : std::uint8_t
{
	Vertex,
	Pixel,
	Geometry,
	Hull,
	Domain,
	Compute,
	RayGeneration,
	Miss,
	ClosestHit,
	AnyHit,
	Intersection,
	Callable,
	Count
};

enum class ShaderStageMask : std::uint16_t
{
	None = 0,
	Vertex = 1 << 0,
	Pixel = 1 << 1,
	Geometry = 1 << 2,
	Hull = 1 << 3,
	Domain = 1 << 4,
	Compute = 1 << 5,
	RayGeneration = 1 << 6,
	Miss = 1 << 7,
	ClosestHit = 1 << 8,
	AnyHit = 1 << 9,
	Intersection = 1 << 10,
	Callable = 1 << 11,
	AllGraphics = (1 << 0) | (1 << 1) | (1 << 2) | (1 << 3) | (1 << 4),
	AllRayTracing = (1 << 6) | (1 << 7) | (1 << 8) | (1 << 9) | (1 << 10) | (1 << 11),
	All = (1 << 12) - 1,
};

SPARKLE_RHI_API ShaderStageMask operator|(ShaderStageMask lhs, ShaderStageMask rhs) noexcept;
SPARKLE_RHI_API ShaderStageMask operator&(ShaderStageMask lhs, ShaderStageMask rhs) noexcept;
SPARKLE_RHI_API ShaderStageMask& operator|=(ShaderStageMask& lhs, ShaderStageMask rhs) noexcept;
SPARKLE_RHI_API bool HasAnyShaderStageMask(ShaderStageMask value, ShaderStageMask flags) noexcept;
SPARKLE_RHI_API ShaderStageMask ToShaderStageMask(ShaderStage stage) noexcept;
SPARKLE_RHI_API const char* GetShaderStagePrefix(ShaderStage stage) noexcept;
SPARKLE_RHI_API std::string FormatShaderStageMask(ShaderStageMask mask);
SPARKLE_RHI_API bool IsRayTracingShaderStage(ShaderStage stage) noexcept;
