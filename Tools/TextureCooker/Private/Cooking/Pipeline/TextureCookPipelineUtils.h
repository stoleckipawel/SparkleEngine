#pragma once

#include "Cooking/Pipeline/TextureCookPipelineTypes.h"
#include "TextureCookRequestList.h"

#include <cstddef>
#include <cstdint>

using AssetAuthoring::TextureCookRequest;
using AssetAuthoring::TextureColorSpace;

namespace TextureCookPipeline
{
	DXGI_FORMAT ResolveUncompressedOutputFormat(const TextureCookRequest& request, bool sourceWasFloat) noexcept;
	DXGI_FORMAT ResolveCompressedOutputFormat(
	    const TextureCookRequest& request,
	    const WorkingImage& workingImage,
	    CompressionTarget target) noexcept;
	TextureFormatIntent ResolveFormatIntent(DXGI_FORMAT format) noexcept;
	DXGI_FORMAT ApplyRequestedColorSpace(DXGI_FORMAT format, TextureColorSpace colorSpace) noexcept;

	bool IsCompressedFormat(DXGI_FORMAT format) noexcept;
	bool IsFloatFormat(DXGI_FORMAT format) noexcept;
	bool IsByteRgbaFormat(DXGI_FORMAT format) noexcept;
	bool IsSrgbCapableFormat(DXGI_FORMAT format) noexcept;
	bool HasMeaningfulAlpha(const WorkingImage& workingImage) noexcept;
	bool IsGreyscaleLike(const WorkingImage& workingImage) noexcept;

	std::uint8_t EncodeByteChannel(float value, bool srgb) noexcept;
	float DecodeByteChannel(std::uint8_t value, bool srgb) noexcept;

	std::uint32_t ComputeBlockCompressedRowPitch(CompressionTarget target, std::uint32_t width) noexcept;
	std::uint32_t ComputeBlockCompressedSlicePitch(CompressionTarget target, std::uint32_t width, std::uint32_t height) noexcept;

	float SampleBilinearWrapped(const WorkingMipLevel& mipLevel, float u, float v, std::size_t channel) noexcept;
	void ComputeCubemapDirection(std::uint32_t faceIndex, float u, float v, float& x, float& y, float& z) noexcept;

	float KaiserKernel(float x, float scale, void* userData);
	float KaiserSupport(float scale, void* userData);
	std::uint16_t FloatToHalf(float value) noexcept;
}
