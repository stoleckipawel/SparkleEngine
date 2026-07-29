#include "PCH.h"

#include "Pipeline/Stages/DecodeStage.h"

#include "Pipeline/FormatPolicy.h"

#include "Core/Public/Diagnostics/Error.h"
#include "Core/Public/Pixel/PixelFormat.h"

#include <cstring>

namespace TextureCookPipeline
{
	static WorkingMipLevel DecodeMipLevel(
	    const TextureMipLevelData& sourceMip,
	    DXGI_FORMAT sourceFormat,
	    TextureColorProcessingPolicy colorProcessingPolicy)
	{
		WorkingMipLevel workingMip;
		workingMip.width = sourceMip.width;
		workingMip.height = sourceMip.height;
		workingMip.pixels.resize(static_cast<std::size_t>(sourceMip.width) * static_cast<std::size_t>(sourceMip.height) * 4u);

		if (IsFloatFormat(sourceFormat))
		{
			if (sourceMip.data.size() != workingMip.pixels.size() * sizeof(float))
			{
				throw Diagnostics::Error("Float source mip payload size does not match its declared dimensions.");
			}

			std::memcpy(workingMip.pixels.data(), sourceMip.data.data(), sourceMip.data.size());
			return workingMip;
		}

		if (!IsByteRgbaFormat(sourceFormat))
		{
			throw Diagnostics::Error("Unsupported uncompressed source texture format for mip processing.");
		}

		if (sourceMip.data.size() != workingMip.pixels.size())
		{
			throw Diagnostics::Error("Byte source mip payload size does not match its declared dimensions.");
		}

		const bool sourceIsBgra = sourceFormat == DXGI_FORMAT_B8G8R8A8_UNORM || sourceFormat == DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
		const bool decodeSrgb = colorProcessingPolicy == TextureColorProcessingPolicy::SrgbLinearize;

		for (std::uint32_t texelIndex = 0; texelIndex < sourceMip.width * sourceMip.height; ++texelIndex)
		{
			const std::size_t sourceOffset = static_cast<std::size_t>(texelIndex) * 4u;
			const std::uint8_t red = sourceIsBgra ? sourceMip.data[sourceOffset + 2u] : sourceMip.data[sourceOffset + 0u];
			const std::uint8_t green = sourceMip.data[sourceOffset + 1u];
			const std::uint8_t blue = sourceIsBgra ? sourceMip.data[sourceOffset + 0u] : sourceMip.data[sourceOffset + 2u];
			const std::uint8_t alpha = sourceMip.data[sourceOffset + 3u];

			workingMip.pixels[sourceOffset + 0u] = Pixel::DecodeByteChannel(red, decodeSrgb);
			workingMip.pixels[sourceOffset + 1u] = Pixel::DecodeByteChannel(green, decodeSrgb);
			workingMip.pixels[sourceOffset + 2u] = Pixel::DecodeByteChannel(blue, decodeSrgb);
			workingMip.pixels[sourceOffset + 3u] = Pixel::DecodeByteChannel(alpha, false);
		}

		return workingMip;
	}

	WorkingTexture BuildWorkingTexture(
	    const TextureCookRequest& request,
	    const TextureLoadResult& sourceTexture)
	{
		if (!IsByteRgbaFormat(sourceTexture.dxgiFormat) && !IsFloatFormat(sourceTexture.dxgiFormat))
		{
			throw Diagnostics::Error("Texture processing only supports uncompressed RGBA8 and RGBA32F source data.");
		}

		const std::uint32_t mipCountToDecode =
		    request.policy.mipPolicy == TextureMipPolicy::PreserveExisting ? sourceTexture.GetMipCount() : 1u;
		WorkingTexture workingTexture;
		workingTexture.dimension = sourceTexture.dimension;
		workingTexture.arraySize = sourceTexture.arraySize;
		workingTexture.sourceWasFloat = IsFloatFormat(sourceTexture.dxgiFormat);
		workingTexture.arraySlices.resize(sourceTexture.arraySlices.size());

		for (std::size_t arraySliceIndex = 0; arraySliceIndex < sourceTexture.arraySlices.size(); ++arraySliceIndex)
		{
			const TextureArraySliceData& sourceSlice = sourceTexture.arraySlices[arraySliceIndex];
			auto& workingSlice = workingTexture.arraySlices[arraySliceIndex];
			workingSlice.reserve(mipCountToDecode);

			for (std::uint32_t mipIndex = 0; mipIndex < mipCountToDecode; ++mipIndex)
			{
				workingSlice.push_back(DecodeMipLevel(
				    sourceSlice.mipLevels[mipIndex],
				    sourceTexture.dxgiFormat,
				    request.policy.colorProcessingPolicy));
			}
		}

		return workingTexture;
	}
}
