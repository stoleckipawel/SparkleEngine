#pragma once

#include "../Formats/PixelFormat.h"

#include <array>

namespace RhiPresentationDefaults
{
	inline constexpr PixelFormat BackBufferFormat = PixelFormat::R8G8B8A8_UNorm;

	inline constexpr std::array<PixelFormat, 4> SupportedBackBufferFormats = {
	    PixelFormat::R8G8B8A8_UNorm,
	    PixelFormat::R8G8B8A8_UNorm_Srgb,
	    PixelFormat::B8G8R8A8_UNorm,
	    PixelFormat::B8G8R8A8_UNorm_Srgb};

	constexpr bool IsSupportedBackBufferFormat(PixelFormat format) noexcept
	{
		for (const PixelFormat supportedFormat : SupportedBackBufferFormats)
		{
			if (format == supportedFormat)
			{
				return true;
			}
		}
		return false;
	}
}

