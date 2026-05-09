#include "PCH.h"

#include "Pipeline/Stages/EncodeStage.h"

#include "Pipeline/Compression/BCCompressor.h"
#include "Pipeline/Compression/CompressionPolicy.h"
#include "Pipeline/FormatPolicy.h"

#include "Core/Public/Pixel/PixelFormat.h"

#include <cstring>

namespace TextureCookPipeline
{
	namespace
	{
		void InitializeOutputTexture(TextureLoadResult& outTexture, const WorkingTexture& workingTexture, DXGI_FORMAT outputFormat)
		{
			outTexture = {};
			outTexture.width = workingTexture.arraySlices.front().front().width;
			outTexture.height = workingTexture.arraySlices.front().front().height;
			outTexture.arraySize = workingTexture.arraySize;
			outTexture.dimension = workingTexture.dimension;
			outTexture.dxgiFormat = outputFormat;
			outTexture.formatIntent = ResolveFormatIntent(outputFormat);
			outTexture.arraySlices.resize(workingTexture.arraySlices.size());
		}

		bool EncodeUncompressedMip(
		    const WorkingMipLevel& sourceMip,
		    bool sourceWasFloat,
		    DXGI_FORMAT outputFormat,
		    TextureMipLevelData& outMip,
		    std::string& outErrorMessage)
		{
			outMip.width = sourceMip.width;
			outMip.height = sourceMip.height;

			if (sourceWasFloat && outputFormat == DXGI_FORMAT_R32G32B32A32_FLOAT)
			{
				outMip.rowPitch = static_cast<std::uint32_t>(sourceMip.width * 4u * sizeof(float));
				outMip.slicePitch = outMip.rowPitch * sourceMip.height;
				outMip.data.resize(outMip.slicePitch);
				std::memcpy(outMip.data.data(), sourceMip.pixels.data(), outMip.data.size());
				outErrorMessage.clear();
				return true;
			}

			outMip.rowPitch = sourceMip.width * 4u;
			outMip.slicePitch = outMip.rowPitch * sourceMip.height;
			outMip.data.resize(outMip.slicePitch);
			const bool srgbOutput = ResolveFormatIntent(outputFormat) == TextureFormatIntent::ColorSrgb;

			for (std::uint32_t texelIndex = 0; texelIndex < sourceMip.width * sourceMip.height; ++texelIndex)
			{
				const std::size_t pixelOffset = static_cast<std::size_t>(texelIndex) * 4u;
				outMip.data[pixelOffset + 0u] = Pixel::EncodeByteChannel(sourceMip.pixels[pixelOffset + 0u], srgbOutput);
				outMip.data[pixelOffset + 1u] = Pixel::EncodeByteChannel(sourceMip.pixels[pixelOffset + 1u], srgbOutput);
				outMip.data[pixelOffset + 2u] = Pixel::EncodeByteChannel(sourceMip.pixels[pixelOffset + 2u], srgbOutput);
				outMip.data[pixelOffset + 3u] = Pixel::EncodeByteChannel(sourceMip.pixels[pixelOffset + 3u], false);
			}

			outErrorMessage.clear();
			return true;
		}

		bool BuildUncompressedTexture(
		    const TextureCookRequest& request,
		    const WorkingTexture& workingTexture,
		    TextureLoadResult& outProcessedTexture,
		    std::string& outErrorMessage)
		{
			const DXGI_FORMAT outputFormat = ResolveUncompressedOutputFormat(request, workingTexture.sourceWasFloat);
			InitializeOutputTexture(outProcessedTexture, workingTexture, outputFormat);

			for (std::size_t arraySliceIndex = 0; arraySliceIndex < workingTexture.arraySlices.size(); ++arraySliceIndex)
			{
				const auto& workingSlice = workingTexture.arraySlices[arraySliceIndex];
				auto& outputSlice = outProcessedTexture.arraySlices[arraySliceIndex];
				outputSlice.mipLevels.reserve(workingSlice.size());

				for (const WorkingMipLevel& workingMip : workingSlice)
				{
					TextureMipLevelData outputMip;
					if (!EncodeUncompressedMip(workingMip, workingTexture.sourceWasFloat, outputFormat, outputMip, outErrorMessage))
					{
						return false;
					}

					outputSlice.mipLevels.push_back(std::move(outputMip));
				}
			}

			outErrorMessage.clear();
			return true;
		}

		bool BuildCompressedTexture(
		    const TextureCookRequest& request,
		    const WorkingTexture& workingTexture,
		    CompressionTarget target,
		    TextureLoadResult& outProcessedTexture,
		    std::string& outErrorMessage)
		{
			const DXGI_FORMAT outputFormat = ResolveCompressedOutputFormat(request, workingTexture, target);
			const bool srgbOutput = ResolveFormatIntent(outputFormat) == TextureFormatIntent::ColorSrgb;

			BCCompressor compressor(target);
			if (!compressor.Initialize(srgbOutput, HasMeaningfulAlpha(workingTexture), outErrorMessage))
			{
				return false;
			}

			InitializeOutputTexture(outProcessedTexture, workingTexture, outputFormat);

			for (std::size_t arraySliceIndex = 0; arraySliceIndex < workingTexture.arraySlices.size(); ++arraySliceIndex)
			{
				const auto& workingSlice = workingTexture.arraySlices[arraySliceIndex];
				auto& outputSlice = outProcessedTexture.arraySlices[arraySliceIndex];
				outputSlice.mipLevels.reserve(workingSlice.size());

				for (const WorkingMipLevel& workingMip : workingSlice)
				{
					TextureMipLevelData outputMip;
					if (!compressor.CompressMip(request, workingMip, outputMip, outErrorMessage))
					{
						return false;
					}

					outputSlice.mipLevels.push_back(std::move(outputMip));
				}
			}

			outErrorMessage.clear();
			return true;
		}
	}

	bool ProcessCompressedSource(
	    const TextureCookRequest& request,
	    TextureLoadResult&& sourceTexture,
	    TextureLoadResult& outProcessedTexture,
	    std::string& outErrorMessage)
	{
		if (request.mipPolicy == TextureMipPolicy::Generate)
		{
			outErrorMessage = "Generating mips from compressed source DDS content is not supported yet.";
			return false;
		}

		if (request.dimension == TextureDimension::TextureCube && !sourceTexture.IsCube())
		{
			outErrorMessage = "Compressed source texture does not contain cubemap data.";
			return false;
		}

		if (request.dimension == TextureDimension::Texture2D && sourceTexture.IsCube())
		{
			outErrorMessage = "Cannot cook cubemap DDS content as a 2D texture.";
			return false;
		}

		outProcessedTexture = std::move(sourceTexture);
		if (request.mipPolicy == TextureMipPolicy::NoMips)
		{
			for (TextureArraySliceData& arraySlice : outProcessedTexture.arraySlices)
			{
				if (arraySlice.mipLevels.size() > 1)
				{
					arraySlice.mipLevels.resize(1);
				}
			}
		}

		outProcessedTexture.dxgiFormat = ApplyRequestedColorSpace(outProcessedTexture.dxgiFormat, request.colorSpace);
		outProcessedTexture.formatIntent = ResolveFormatIntent(outProcessedTexture.dxgiFormat);
		outErrorMessage.clear();
		return true;
	}

	bool BuildOutputTexture(
	    const TextureCookRequest& request,
	    const WorkingTexture& workingTexture,
	    TextureLoadResult& outProcessedTexture,
	    std::string& outErrorMessage)
	{
		const CompressionTarget target = ResolveCompressionTarget(request, workingTexture);
		if (target == CompressionTarget::None)
		{
			return BuildUncompressedTexture(request, workingTexture, outProcessedTexture, outErrorMessage);
		}

		return BuildCompressedTexture(request, workingTexture, target, outProcessedTexture, outErrorMessage);
	}
}