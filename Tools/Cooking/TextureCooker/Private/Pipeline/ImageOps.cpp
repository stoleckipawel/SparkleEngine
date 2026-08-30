#include "PCH.h"

#include "Pipeline/ImageOps.h"

#include "Core/Public/Diagnostics/Error.h"
#include "Core/Public/Math/SignalProcessing.h"

#include <algorithm>
#include <cmath>
#include <optional>

namespace TextureCookPipeline
{
	float KaiserKernel(float x, float, void*)
	{
		constexpr float support = 3.0f;
		constexpr float beta = 6.5f;
		const float absoluteX = std::fabs(x);
		if (absoluteX >= support)
		{
			return 0.0f;
		}

		const float ratio = absoluteX / support;
		const float window = MathUtils::BesselI0(beta * std::sqrt((std::max) (0.0f, 1.0f - (ratio * ratio)))) / MathUtils::BesselI0(beta);
		return MathUtils::Sinc(x) * window;
	}

	float KaiserSupport(float, void*)
	{
		return 3.0f;
	}

	static std::optional<std::size_t> ResolveChannelIndex(TextureChannelMask channelMask) noexcept
	{
		switch (channelMask)
		{
			case TextureChannelMask::Red:
				return 0u;
			case TextureChannelMask::Green:
				return 1u;
			case TextureChannelMask::Blue:
				return 2u;
			case TextureChannelMask::Alpha:
				return 3u;
			case TextureChannelMask::Rgba:
				return std::nullopt;
		}

		return std::nullopt;
	}

	void ExtractChannel(WorkingTexture& workingTexture, TextureChannelMask channelMask)
	{
		const std::optional<std::size_t> channelIndex = ResolveChannelIndex(channelMask);
		if (!channelIndex)
		{
			return;
		}

		for (auto& arraySlice : workingTexture.arraySlices)
		{
			for (WorkingMipLevel& mipLevel : arraySlice)
			{
				if (mipLevel.pixels.size() != static_cast<std::size_t>(mipLevel.width) * static_cast<std::size_t>(mipLevel.height) * 4u)
				{
					throw Diagnostics::Error("Working texture mip payload size is invalid during channel extraction.");
				}

				for (std::size_t pixelOffset = 0; pixelOffset < mipLevel.pixels.size(); pixelOffset += 4u)
				{
					const float value = mipLevel.pixels[pixelOffset + *channelIndex];
					mipLevel.pixels[pixelOffset + 0u] = value;
					mipLevel.pixels[pixelOffset + 1u] = value;
					mipLevel.pixels[pixelOffset + 2u] = value;
					mipLevel.pixels[pixelOffset + 3u] = 1.0f;
				}
			}
		}
	}
}
