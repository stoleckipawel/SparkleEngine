#pragma once

#include "RHI/Public/Formats/PixelFormat.h"

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

struct TextureRuntimeInfo final
{
	std::uint32_t Width = 0;
	std::uint32_t Height = 0;
	std::uint32_t ArraySize = 0;
	TextureResourceDimension Dimension = TextureResourceDimension::Texture2D;
	PixelFormat Format = PixelFormat::Unknown;
	TextureFormatIntent FormatIntent = TextureFormatIntent::Unknown;
	std::uint16_t MipCount = 0;
	std::uint64_t EstimatedByteSize = 0;
	std::uint64_t GpuShaderResourceViewId = 0;
	bool IsValid = false;
};
