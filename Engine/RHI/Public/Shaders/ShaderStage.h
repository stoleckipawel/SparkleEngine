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
	Count
};

enum class ShaderStageMask : std::uint8_t
{
	None = 0,
	Vertex = 1 << 0,
	Pixel = 1 << 1,
	Geometry = 1 << 2,
	Hull = 1 << 3,
	Domain = 1 << 4,
	Compute = 1 << 5,
	AllGraphics = (1 << 0) | (1 << 1) | (1 << 2) | (1 << 3) | (1 << 4),
	All = (1 << 0) | (1 << 1) | (1 << 2) | (1 << 3) | (1 << 4) | (1 << 5),
};

SPARKLE_RHI_API ShaderStageMask operator|(ShaderStageMask lhs, ShaderStageMask rhs) noexcept;
SPARKLE_RHI_API ShaderStageMask operator&(ShaderStageMask lhs, ShaderStageMask rhs) noexcept;
SPARKLE_RHI_API ShaderStageMask& operator|=(ShaderStageMask& lhs, ShaderStageMask rhs) noexcept;
SPARKLE_RHI_API bool HasAnyShaderStageMask(ShaderStageMask value, ShaderStageMask flags) noexcept;
SPARKLE_RHI_API ShaderStageMask ToShaderStageMask(ShaderStage stage) noexcept;
SPARKLE_RHI_API const char* GetShaderStagePrefix(ShaderStage stage) noexcept;
SPARKLE_RHI_API std::string FormatShaderStageMask(ShaderStageMask mask);
