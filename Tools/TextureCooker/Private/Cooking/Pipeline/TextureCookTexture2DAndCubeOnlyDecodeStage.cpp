#include "PCH.h"

#include "Cooking/Pipeline/TextureCookTexture2DAndCubeOnlyDecodeStage.h"

#include "Cooking/Pipeline/TextureCookPipelineUtils.h"

#include "Core/Public/Pixel/PixelFormat.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <numbers>

using AssetAuthoring::TextureCookRequest;
using AssetAuthoring::TextureColorSpace;

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

		bool DecodeToWorkingImage(
		    const TextureCookRequest& request,
		    const TextureLoadResult& sourceTexture,
		    WorkingImage& outWorkingImage,
		    std::string& outErrorMessage)
		{
			if (!IsByteRgbaFormat(sourceTexture.dxgiFormat) && !IsFloatFormat(sourceTexture.dxgiFormat))
			{
				outErrorMessage = "Texture processing only supports uncompressed RGBA8 and RGBA32F source data.";
				return false;
			}

			const std::uint32_t mipCountToDecode = request.mipPolicy == TextureMipPolicy::PreserveExisting ? sourceTexture.GetMipCount() : 1u;
			outWorkingImage.dimension = sourceTexture.dimension;
			outWorkingImage.arraySize = sourceTexture.arraySize;
			outWorkingImage.sourceWasFloat = IsFloatFormat(sourceTexture.dxgiFormat);
			outWorkingImage.arraySlices.clear();
			outWorkingImage.arraySlices.resize(sourceTexture.arraySlices.size());

			for (std::size_t arraySliceIndex = 0; arraySliceIndex < sourceTexture.arraySlices.size(); ++arraySliceIndex)
			{
				const TextureArraySliceData& sourceSlice = sourceTexture.arraySlices[arraySliceIndex];
				auto& workingSlice = outWorkingImage.arraySlices[arraySliceIndex];
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

		bool ProjectEquirectangularToCubemap(WorkingImage& workingImage, std::string& outErrorMessage)
		{
			if (workingImage.arraySize != 1 || workingImage.arraySlices.size() != 1 || workingImage.arraySlices.front().empty())
			{
				outErrorMessage = "Equirectangular cubemap projection requires a single-surface 2D source texture.";
				return false;
			}

			const WorkingMipLevel& sourceMip = workingImage.arraySlices.front().front();
			const std::uint32_t faceSize = (std::max)(1u, (std::min)(sourceMip.width / 4u, sourceMip.height / 2u));

			std::vector<std::vector<WorkingMipLevel>> cubeSlices(6);
			for (std::uint32_t faceIndex = 0; faceIndex < 6u; ++faceIndex)
			{
				WorkingMipLevel faceMip;
				faceMip.width = faceSize;
				faceMip.height = faceSize;
				faceMip.pixels.resize(static_cast<std::size_t>(faceSize) * static_cast<std::size_t>(faceSize) * 4u);

				for (std::uint32_t y = 0; y < faceSize; ++y)
				{
					for (std::uint32_t x = 0; x < faceSize; ++x)
					{
						const float u = (2.0f * (static_cast<float>(x) + 0.5f) / static_cast<float>(faceSize)) - 1.0f;
						const float v = (2.0f * (static_cast<float>(y) + 0.5f) / static_cast<float>(faceSize)) - 1.0f;

						float dirX = 0.0f;
						float dirY = 0.0f;
						float dirZ = 0.0f;
						ComputeCubemapDirection(faceIndex, u, v, dirX, dirY, dirZ);
						const float length = std::sqrt(dirX * dirX + dirY * dirY + dirZ * dirZ);
						dirX /= length;
						dirY /= length;
						dirZ /= length;

						const float longitude = std::atan2(dirZ, dirX);
						const float latitude = std::asin((std::clamp)(dirY, -1.0f, 1.0f));
						const float sampleU = 0.5f + (longitude / (2.0f * std::numbers::pi_v<float>));
						const float sampleV = 0.5f - (latitude / std::numbers::pi_v<float>);

						const std::size_t pixelOffset = (static_cast<std::size_t>(y) * faceSize + x) * 4u;
						for (std::size_t channel = 0; channel < 4u; ++channel)
						{
							faceMip.pixels[pixelOffset + channel] = SampleBilinearWrapped(sourceMip, sampleU, sampleV, channel);
						}
					}
				}

				cubeSlices[faceIndex].push_back(std::move(faceMip));
			}

			workingImage.dimension = TextureResourceDimension::TextureCube;
			workingImage.arraySize = 6;
			workingImage.arraySlices = std::move(cubeSlices);
			outErrorMessage.clear();
			return true;
		}

		bool TransformToRequestedDimension(const TextureCookRequest& request, WorkingImage& workingImage, std::string& outErrorMessage)
		{
			if (request.dimension == TextureDimension::Texture2D)
			{
				if (workingImage.dimension != TextureResourceDimension::Texture2D)
				{
					outErrorMessage = "Cannot cook cubemap source content as a 2D texture.";
					return false;
				}

				outErrorMessage.clear();
				return true;
			}

			if (workingImage.dimension == TextureResourceDimension::TextureCube)
			{
				outErrorMessage.clear();
				return true;
			}

			return ProjectEquirectangularToCubemap(workingImage, outErrorMessage);
		}
	}

	bool BuildWorkingImage(
	    const TextureCookRequest& request,
	    const TextureLoadResult& sourceTexture,
	    WorkingImage& outWorkingImage,
	    std::string& outErrorMessage)
	{
		if (!DecodeToWorkingImage(request, sourceTexture, outWorkingImage, outErrorMessage))
		{
			return false;
		}

		return TransformToRequestedDimension(request, outWorkingImage, outErrorMessage);
	}
}