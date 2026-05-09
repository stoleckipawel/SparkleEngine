#pragma once

#include <cstdint>

enum class TextureGroup : std::uint8_t
{
	Default = 0,
	Diffuse = 1,
	NormalMap = 2,
	Roughness = 3,
	Metallic = 4,
	AmbientOcclusion = 5,
	Emissive = 6,
	SubsurfaceColor = 7,
	SubsurfaceStrength = 8,
	HdrColor = 9,
};

constexpr std::uint32_t GetTextureGroupFlag(TextureGroup textureGroup) noexcept
{
	return textureGroup == TextureGroup::Default ? 0u : (1u << static_cast<std::uint8_t>(textureGroup));
}