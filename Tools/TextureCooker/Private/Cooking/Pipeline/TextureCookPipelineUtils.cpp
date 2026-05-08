#include "PCH.h"

#include "Cooking/Pipeline/TextureCookPipelineUtils.h"

#include <algorithm>
#include <bit>
#include <cmath>
#include <numbers>

namespace TextureCookPipeline
{
	DXGI_FORMAT ResolveUncompressedOutputFormat(const TextureCookRequest& request, bool sourceWasFloat) noexcept
	{
		if (sourceWasFloat)
		{
			return DXGI_FORMAT_R32G32B32A32_FLOAT;
		}

		return request.colorSpace == TextureColorSpace::Srgb ? DXGI_FORMAT_R8G8B8A8_UNORM_SRGB : DXGI_FORMAT_R8G8B8A8_UNORM;
	}

	DXGI_FORMAT ResolveCompressedOutputFormat(
	    const TextureCookRequest& request,
	    const WorkingImage& workingImage,
	    CompressionTarget target) noexcept
	{
		switch (target)
		{
			case CompressionTarget::BC1:
				return request.colorSpace == TextureColorSpace::Srgb ? DXGI_FORMAT_BC1_UNORM_SRGB : DXGI_FORMAT_BC1_UNORM;
			case CompressionTarget::BC4:
				return DXGI_FORMAT_BC4_UNORM;
			case CompressionTarget::BC5:
				return DXGI_FORMAT_BC5_UNORM;
			case CompressionTarget::BC6H:
				return DXGI_FORMAT_BC6H_UF16;
			case CompressionTarget::BC7:
				if (request.compressionFamilyPreference == TextureCompressionFamilyPreference::Masks)
				{
					return DXGI_FORMAT_BC7_UNORM;
				}

				return request.colorSpace == TextureColorSpace::Srgb ? DXGI_FORMAT_BC7_UNORM_SRGB : DXGI_FORMAT_BC7_UNORM;
			case CompressionTarget::None:
			default:
				return ResolveUncompressedOutputFormat(request, workingImage.sourceWasFloat);
		}
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
				case DXGI_FORMAT_R8G8B8A8_UNORM: return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
				case DXGI_FORMAT_B8G8R8A8_UNORM: return DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
				case DXGI_FORMAT_BC1_UNORM: return DXGI_FORMAT_BC1_UNORM_SRGB;
				case DXGI_FORMAT_BC2_UNORM: return DXGI_FORMAT_BC2_UNORM_SRGB;
				case DXGI_FORMAT_BC3_UNORM: return DXGI_FORMAT_BC3_UNORM_SRGB;
				case DXGI_FORMAT_BC7_UNORM: return DXGI_FORMAT_BC7_UNORM_SRGB;
				default: return format;
			}
		}

		switch (format)
		{
			case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB: return DXGI_FORMAT_R8G8B8A8_UNORM;
			case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB: return DXGI_FORMAT_B8G8R8A8_UNORM;
			case DXGI_FORMAT_BC1_UNORM_SRGB: return DXGI_FORMAT_BC1_UNORM;
			case DXGI_FORMAT_BC2_UNORM_SRGB: return DXGI_FORMAT_BC2_UNORM;
			case DXGI_FORMAT_BC3_UNORM_SRGB: return DXGI_FORMAT_BC3_UNORM;
			case DXGI_FORMAT_BC7_UNORM_SRGB: return DXGI_FORMAT_BC7_UNORM;
			default: return format;
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

	bool HasMeaningfulAlpha(const WorkingImage& workingImage) noexcept
	{
		const WorkingMipLevel& topMip = workingImage.arraySlices.front().front();
		for (std::size_t pixelOffset = 3; pixelOffset < topMip.pixels.size(); pixelOffset += 4u)
		{
			if (topMip.pixels[pixelOffset] < 0.999f)
			{
				return true;
			}
		}

		return false;
	}

	bool IsGreyscaleLike(const WorkingImage& workingImage) noexcept
	{
		const WorkingMipLevel& topMip = workingImage.arraySlices.front().front();
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

	std::uint8_t EncodeByteChannel(float value, bool srgb) noexcept
	{
		const float clampedValue = (std::clamp)(value, 0.0f, 1.0f);
		const float encodedValue = srgb ? (clampedValue <= 0.0031308f ? clampedValue * 12.92f
		                                                             : 1.055f * std::pow(clampedValue, 1.0f / 2.4f) - 0.055f)
		                                : clampedValue;
		return static_cast<std::uint8_t>((std::clamp)(std::lround(encodedValue * 255.0f), 0l, 255l));
	}

	float DecodeByteChannel(std::uint8_t value, bool srgb) noexcept
	{
		const float normalizedValue = static_cast<float>(value) / 255.0f;
		if (!srgb)
		{
			return normalizedValue;
		}

		return normalizedValue <= 0.04045f ? normalizedValue / 12.92f : std::pow((normalizedValue + 0.055f) / 1.055f, 2.4f);
	}

	std::uint32_t ComputeBlockCompressedRowPitch(CompressionTarget target, std::uint32_t width) noexcept
	{
		const std::uint32_t blockBytes = target == CompressionTarget::BC1 || target == CompressionTarget::BC4 ? 8u : 16u;
		return (std::max)(1u, (width + 3u) / 4u) * blockBytes;
	}

	std::uint32_t ComputeBlockCompressedSlicePitch(CompressionTarget target, std::uint32_t width, std::uint32_t height) noexcept
	{
		return ComputeBlockCompressedRowPitch(target, width) * (std::max)(1u, (height + 3u) / 4u);
	}

	float SampleBilinearWrapped(const WorkingMipLevel& mipLevel, float u, float v, std::size_t channel) noexcept
	{
		u -= std::floor(u);
		v = (std::clamp)(v, 0.0f, 1.0f);

		const float x = u * static_cast<float>(mipLevel.width) - 0.5f;
		const float y = v * static_cast<float>(mipLevel.height) - 0.5f;
		const int x0 = static_cast<int>(std::floor(x));
		const int y0 = static_cast<int>(std::floor(y));
		const int x1 = x0 + 1;
		const int y1 = y0 + 1;
		const float tx = x - static_cast<float>(x0);
		const float ty = y - static_cast<float>(y0);

		auto sample = [&](int sampleX, int sampleY) -> float
		{
			const std::uint32_t wrappedX = static_cast<std::uint32_t>(
			    (sampleX % static_cast<int>(mipLevel.width) + static_cast<int>(mipLevel.width)) % static_cast<int>(mipLevel.width));
			const std::uint32_t clampedY = static_cast<std::uint32_t>((std::clamp)(sampleY, 0, static_cast<int>(mipLevel.height) - 1));
			const std::size_t sampleOffset = (static_cast<std::size_t>(clampedY) * mipLevel.width + wrappedX) * 4u;
			return mipLevel.pixels[sampleOffset + channel];
		};

		const float c00 = sample(x0, y0);
		const float c10 = sample(x1, y0);
		const float c01 = sample(x0, y1);
		const float c11 = sample(x1, y1);
		const float top = c00 + ((c10 - c00) * tx);
		const float bottom = c01 + ((c11 - c01) * tx);
		return top + ((bottom - top) * ty);
	}

	void ComputeCubemapDirection(std::uint32_t faceIndex, float u, float v, float& x, float& y, float& z) noexcept
	{
		switch (faceIndex)
		{
			case 0u:
				x = 1.0f;
				y = -v;
				z = -u;
				return;
			case 1u:
				x = -1.0f;
				y = -v;
				z = u;
				return;
			case 2u:
				x = u;
				y = 1.0f;
				z = v;
				return;
			case 3u:
				x = u;
				y = -1.0f;
				z = -v;
				return;
			case 4u:
				x = u;
				y = -v;
				z = 1.0f;
				return;
			default:
				x = -u;
				y = -v;
				z = -1.0f;
				return;
		}
	}

	namespace
	{
		float BesselI0(float value) noexcept
		{
			float sum = 1.0f;
			float term = 1.0f;
			const float halfValue = value * 0.5f;
			for (int index = 1; index < 8; ++index)
			{
				term *= (halfValue * halfValue) / static_cast<float>(index * index);
				sum += term;
			}

			return sum;
		}

		float Sinc(float value) noexcept
		{
			if (std::fabs(value) <= 1e-6f)
			{
				return 1.0f;
			}

			const float angle = std::numbers::pi_v<float> * value;
			return std::sin(angle) / angle;
		}
	}

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
		const float window = BesselI0(beta * std::sqrt((std::max)(0.0f, 1.0f - (ratio * ratio)))) / BesselI0(beta);
		return Sinc(x) * window;
	}

	float KaiserSupport(float, void*)
	{
		return 3.0f;
	}

	std::uint16_t FloatToHalf(float value) noexcept
	{
		const std::uint32_t bits = std::bit_cast<std::uint32_t>(value);
		const std::uint32_t sign = (bits >> 16u) & 0x8000u;
		std::int32_t exponent = static_cast<std::int32_t>((bits >> 23u) & 0xffu) - 127 + 15;
		std::uint32_t mantissa = bits & 0x7fffffu;

		if (exponent <= 0)
		{
			if (exponent < -10)
			{
				return static_cast<std::uint16_t>(sign);
			}

			mantissa = (mantissa | 0x800000u) >> (1 - exponent);
			return static_cast<std::uint16_t>(sign | ((mantissa + 0x1000u) >> 13u));
		}

		if (exponent >= 31)
		{
			return static_cast<std::uint16_t>(sign | 0x7c00u);
		}

		return static_cast<std::uint16_t>(sign | (static_cast<std::uint32_t>(exponent) << 10u) | ((mantissa + 0x1000u) >> 13u));
	}
}
