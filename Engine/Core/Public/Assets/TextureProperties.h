#pragma once

#include <cstdint>

enum class TextureColorSpace : std::uint8_t
{
	Linear = 0,
	Srgb = 1,
};

enum class TextureDimension : std::uint8_t
{
	Texture2D = 0,
	TextureCube = 1,
};

enum class TextureChannelMask : std::uint8_t
{
	Rgba = 0,
	Red = 1,
	Green = 2,
	Blue = 3,
	Alpha = 4,
};