#include "PCH.h"

#include "Pipeline/FormatPolicy.h"

#include <algorithm>
#include <cmath>

namespace TextureCookPipeline
{
	DXGI_FORMAT ResolveUncompressedOutputFormat(const TextureCookRequest& request, bool sourceWasFloat) noexcept
	{
		if (sourceWasFloat)
		{
			return DXGI_FORMAT_R32G32B32A32_FLOAT;
		}

		return request.policy.colorSpace == TextureColorSpace::Srgb ? DXGI_FORMAT_R8G8B8A8_UNORM_SRGB : DXGI_FORMAT_R8G8B8A8_UNORM;
	}

	TextureFormatIntent ResolveFormatIntent(DXGI_FORMAT format) noexcept
	{
		switch (format)
		{
			case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
			case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
			case DXGI_FORMAT_BC1_UNORM_SRGB:
			case DXGI_FORMAT_BC2_UNORM_SRGB:
			case DXGI_FORMAT_BC3_UNORM_SRGB:
			case DXGI_FORMAT_BC7_UNORM_SRGB:
				return TextureFormatIntent::ColorSrgb;
			default:
				return TextureFormatIntent::DataLinear;
		}
	}

	DXGI_FORMAT ApplyRequestedColorSpace(DXGI_FORMAT format, TextureColorSpace colorSpace) noexcept
	{
		if (!IsSrgbCapableFormat(format))
		{
			return format;
		}

		if (colorSpace == TextureColorSpace::Srgb)
		{
			switch (format)
			{
				case DXGI_FORMAT_R8G8B8A8_UNORM:
					return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
				case DXGI_FORMAT_B8G8R8A8_UNORM:
					return DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
				case DXGI_FORMAT_BC1_UNORM:
					return DXGI_FORMAT_BC1_UNORM_SRGB;
				case DXGI_FORMAT_BC2_UNORM:
					return DXGI_FORMAT_BC2_UNORM_SRGB;
				case DXGI_FORMAT_BC3_UNORM:
					return DXGI_FORMAT_BC3_UNORM_SRGB;
				case DXGI_FORMAT_BC7_UNORM:
					return DXGI_FORMAT_BC7_UNORM_SRGB;
				default:
					return format;
			}
		}

		switch (format)
		{
			case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
				return DXGI_FORMAT_R8G8B8A8_UNORM;
			case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
				return DXGI_FORMAT_B8G8R8A8_UNORM;
			case DXGI_FORMAT_BC1_UNORM_SRGB:
				return DXGI_FORMAT_BC1_UNORM;
			case DXGI_FORMAT_BC2_UNORM_SRGB:
				return DXGI_FORMAT_BC2_UNORM;
			case DXGI_FORMAT_BC3_UNORM_SRGB:
				return DXGI_FORMAT_BC3_UNORM;
			case DXGI_FORMAT_BC7_UNORM_SRGB:
				return DXGI_FORMAT_BC7_UNORM;
			default:
				return format;
		}
	}

	bool IsCompressedFormat(DXGI_FORMAT format) noexcept
	{
		switch (format)
		{
			case DXGI_FORMAT_BC1_UNORM:
			case DXGI_FORMAT_BC1_UNORM_SRGB:
			case DXGI_FORMAT_BC2_UNORM:
			case DXGI_FORMAT_BC2_UNORM_SRGB:
			case DXGI_FORMAT_BC3_UNORM:
			case DXGI_FORMAT_BC3_UNORM_SRGB:
			case DXGI_FORMAT_BC4_UNORM:
			case DXGI_FORMAT_BC4_SNORM:
			case DXGI_FORMAT_BC5_UNORM:
			case DXGI_FORMAT_BC5_SNORM:
			case DXGI_FORMAT_BC6H_UF16:
			case DXGI_FORMAT_BC7_UNORM:
			case DXGI_FORMAT_BC7_UNORM_SRGB:
				return true;
			default:
				return false;
		}
	}

	bool IsFloatFormat(DXGI_FORMAT format) noexcept
	{
		return format == DXGI_FORMAT_R32G32B32A32_FLOAT;
	}

	bool IsByteRgbaFormat(DXGI_FORMAT format) noexcept
	{
		switch (format)
		{
			case DXGI_FORMAT_R8G8B8A8_UNORM:
			case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
			case DXGI_FORMAT_B8G8R8A8_UNORM:
			case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
				return true;
			default:
				return false;
		}
	}

	bool IsSrgbCapableFormat(DXGI_FORMAT format) noexcept
	{
		switch (format)
		{
			case DXGI_FORMAT_R8G8B8A8_UNORM:
			case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
			case DXGI_FORMAT_B8G8R8A8_UNORM:
			case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
			case DXGI_FORMAT_BC1_UNORM:
			case DXGI_FORMAT_BC1_UNORM_SRGB:
			case DXGI_FORMAT_BC2_UNORM:
			case DXGI_FORMAT_BC2_UNORM_SRGB:
			case DXGI_FORMAT_BC3_UNORM:
			case DXGI_FORMAT_BC3_UNORM_SRGB:
			case DXGI_FORMAT_BC7_UNORM:
			case DXGI_FORMAT_BC7_UNORM_SRGB:
				return true;
			default:
				return false;
		}
	}

	bool HasMeaningfulAlpha(const WorkingTexture& workingTexture) noexcept
	{
		const WorkingMipLevel& topMip = workingTexture.arraySlices.front().front();
		for (std::size_t pixelOffset = 3; pixelOffset < topMip.pixels.size(); pixelOffset += 4u)
		{
			if (topMip.pixels[pixelOffset] < 0.999f)
			{
				return true;
			}
		}

		return false;
	}

	bool IsGreyscaleLike(const WorkingTexture& workingTexture) noexcept
	{
		const WorkingMipLevel& topMip = workingTexture.arraySlices.front().front();
		for (std::size_t pixelOffset = 0; pixelOffset < topMip.pixels.size(); pixelOffset += 4u)
		{
			const float red = topMip.pixels[pixelOffset + 0u];
			const float green = topMip.pixels[pixelOffset + 1u];
			const float blue = topMip.pixels[pixelOffset + 2u];
			if (std::fabs(red - green) > 1e-4f || std::fabs(red - blue) > 1e-4f)
			{
				return false;
			}
		}

		return true;
	}
}