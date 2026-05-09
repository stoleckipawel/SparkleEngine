#include "PCH.h"

#include "Pipeline/Stages/DecodeStage.h"

#include "Pipeline/FormatPolicy.h"

#include "Core/Public/Pixel/PixelFormat.h"

#include <cstring>

namespace TextureCookPipeline
{
	namespace
	{
		bool DecodeMipLevel(
		    const TextureMipLevelData& sourceMip,
		    DXGI_FORMAT sourceFormat,
		    TextureColorProcessingPolicy colorProcessingPolicy,
		    WorkingMipLevel& outWorkingMip,
		    std::string& outErrorMessage)
		{
			outWorkingMip.width = sourceMip.width;
			outWorkingMip.height = sourceMip.height;
			outWorkingMip.pixels.resize(static_cast<std::size_t>(sourceMip.width) * static_cast<std::size_t>(sourceMip.height) * 4u);

			if (IsFloatFormat(sourceFormat))
			{
				if (sourceMip.data.size() != outWorkingMip.pixels.size() * sizeof(float))
				{
					outErrorMessage = "Float source mip payload size does not match its declared dimensions.";
					return false;
				}

				std::memcpy(outWorkingMip.pixels.data(), sourceMip.data.data(), sourceMip.data.size());
				outErrorMessage.clear();
				return true;
			}

			if (!IsByteRgbaFormat(sourceFormat))
			{
				outErrorMessage = "Unsupported uncompressed source texture format for mip processing.";
				return false;
			}

			const bool sourceIsBgra = sourceFormat == DXGI_FORMAT_B8G8R8A8_UNORM || sourceFormat == DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
			const bool applySrgb = colorProcessingPolicy == TextureColorProcessingPolicy::SrgbLinearize;

			for (std::uint32_t texelIndex = 0; texelIndex < sourceMip.width * sourceMip.height; ++texelIndex)
			{
				const std::size_t sourceOffset = static_cast<std::size_t>(texelIndex) * 4u;
				const std::uint8_t red = sourceIsBgra ? sourceMip.data[sourceOffset + 2u] : sourceMip.data[sourceOffset + 0u];
				const std::uint8_t green = sourceIsBgra ? sourceMip.data[sourceOffset + 1u] : sourceMip.data[sourceOffset + 1u];
				const std::uint8_t blue = sourceIsBgra ? sourceMip.data[sourceOffset + 0u] : sourceMip.data[sourceOffset + 2u];
				const std::uint8_t alpha = sourceMip.data[sourceOffset + 3u];

				outWorkingMip.pixels[sourceOffset + 0u] = Pixel::DecodeByteChannel(red, applySrgb);
				outWorkingMip.pixels[sourceOffset + 1u] = Pixel::DecodeByteChannel(green, applySrgb);
				outWorkingMip.pixels[sourceOffset + 2u] = Pixel::DecodeByteChannel(blue, applySrgb);
				outWorkingMip.pixels[sourceOffset + 3u] = Pixel::DecodeByteChannel(alpha, false);
			}

			outErrorMessage.clear();
			return true;
		}

		bool DecodeToWorkingTexture(
		    const TextureCookRequest& request,
		    const TextureLoadResult& sourceTexture,
		    WorkingTexture& outWorkingTexture,
		    std::string& outErrorMessage)
		{
			if (!IsByteRgbaFormat(sourceTexture.dxgiFormat) && !IsFloatFormat(sourceTexture.dxgiFormat))
			{
				outErrorMessage = "Texture processing only supports uncompressed RGBA8 and RGBA32F source data.";
				return false;
			}

			const std::uint32_t mipCountToDecode =
			    request.mipPolicy == TextureMipPolicy::PreserveExisting ? sourceTexture.GetMipCount() : 1u;
			outWorkingTexture.dimension = sourceTexture.dimension;
			outWorkingTexture.arraySize = sourceTexture.arraySize;
			outWorkingTexture.sourceWasFloat = IsFloatFormat(sourceTexture.dxgiFormat);
			outWorkingTexture.arraySlices.clear();
			outWorkingTexture.arraySlices.resize(sourceTexture.arraySlices.size());

			for (std::size_t arraySliceIndex = 0; arraySliceIndex < sourceTexture.arraySlices.size(); ++arraySliceIndex)
			{
				const TextureArraySliceData& sourceSlice = sourceTexture.arraySlices[arraySliceIndex];
				auto& workingSlice = outWorkingTexture.arraySlices[arraySliceIndex];
				workingSlice.reserve(mipCountToDecode);

				for (std::uint32_t mipIndex = 0; mipIndex < mipCountToDecode; ++mipIndex)
				{
					WorkingMipLevel workingMip;
					if (!DecodeMipLevel(
					        sourceSlice.mipLevels[mipIndex],
					        sourceTexture.dxgiFormat,
					        request.colorProcessingPolicy,
					        workingMip,
					        outErrorMessage))
					{
						return false;
					}

					workingSlice.push_back(std::move(workingMip));
				}
			}

			outErrorMessage.clear();
			return true;
		}

	}

	bool BuildWorkingTexture(
	    const TextureCookRequest& request,
	    const TextureLoadResult& sourceTexture,
	    WorkingTexture& outWorkingTexture,
	    std::string& outErrorMessage)
	{
		return DecodeToWorkingTexture(request, sourceTexture, outWorkingTexture, outErrorMessage);
	}
}