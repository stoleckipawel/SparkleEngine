#pragma once

#include <cstdint>

enum class TextureFormatIntent : std::uint8_t
{
	Unknown,
	ColorSrgb,
	DataLinear
};

enum class TextureResourceDimension : std::uint8_t
{
	Texture2D = 0,
	TextureCube = 1,
};
