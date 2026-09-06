#pragma once

#include "../Formats/PixelFormat.h"
#include "../Frame/RhiFrameConstants.h"

#include <cstdint>
#include <array>

namespace RhiPresentationDefaults
{
	inline constexpr PixelFormat DefaultBackBufferFormat = PixelFormat::R8G8B8A8_UNorm;
	inline constexpr std::uint32_t DefaultBackBufferCount = 3u;
	inline constexpr std::uint32_t MinBackBufferCount = 2u;
	inline constexpr std::uint32_t MaxBackBufferCount = 3u;
	inline constexpr std::uint32_t DefaultMaximumFramesInFlight = 2u;
	inline constexpr std::uint32_t MinFramesInFlight = 1u;
	inline constexpr std::uint32_t MaxFramesInFlight = RhiFrameConstants::MaxFrameSlotCount;

	inline constexpr std::array<PixelFormat, 4> SupportedBackBufferFormats =
	    {PixelFormat::R8G8B8A8_UNorm, PixelFormat::R8G8B8A8_UNorm_Srgb, PixelFormat::B8G8R8A8_UNorm, PixelFormat::B8G8R8A8_UNorm_Srgb};

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

struct RhiPresentationConfiguration final
{
	std::uint32_t BackBufferCount = RhiPresentationDefaults::DefaultBackBufferCount;
	std::uint32_t MaximumFramesInFlight = RhiPresentationDefaults::DefaultMaximumFramesInFlight;
};
