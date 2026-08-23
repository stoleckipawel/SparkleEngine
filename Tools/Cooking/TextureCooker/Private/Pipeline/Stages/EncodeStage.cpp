#include "PCH.h"

#include "Pipeline/Stages/EncodeStage.h"

#include "Pipeline/Compression/BCCompressor.h"
#include "Pipeline/Compression/CompressionPolicy.h"
#include "Pipeline/FormatPolicy.h"

#include "Core/Public/Diagnostics/Error.h"
#include "Core/Public/Pixel/PixelFormat.h"

#include <cstring>

namespace TextureCookPipeline
{
	static TextureLoadResult BuildOutputLayout(const WorkingTexture& workingTexture, DXGI_FORMAT outputFormat)
	{
		TextureLoadResult texture;
		texture.width = workingTexture.arraySlices.front().front().width;
		texture.height = workingTexture.arraySlices.front().front().height;
		texture.arraySize = workingTexture.arraySize;
		texture.dimension = workingTexture.dimension;
		texture.dxgiFormat = outputFormat;
		texture.formatIntent = ResolveFormatIntent(outputFormat);
		texture.arraySlices.resize(workingTexture.arraySlices.size());
		return texture;
	}

	static TextureMipLevelData EncodeUncompressedMip(const WorkingMipLevel& sourceMip, bool sourceWasFloat, DXGI_FORMAT outputFormat)
	{
		TextureMipLevelData mip;
		mip.width = sourceMip.width;
		mip.height = sourceMip.height;

		if (sourceWasFloat && outputFormat == DXGI_FORMAT_R32G32B32A32_FLOAT)
		{
			mip.rowPitch = static_cast<std::uint32_t>(sourceMip.width * 4u * sizeof(float));
			mip.slicePitch = mip.rowPitch * sourceMip.height;
			mip.data.resize(mip.slicePitch);
			std::memcpy(mip.data.data(), sourceMip.pixels.data(), mip.data.size());
			return mip;
		}

		mip.rowPitch = sourceMip.width * 4u;
		mip.slicePitch = mip.rowPitch * sourceMip.height;
		mip.data.resize(mip.slicePitch);
		const bool srgbOutput = ResolveFormatIntent(outputFormat) == TextureFormatIntent::ColorSrgb;

		for (std::uint32_t texelIndex = 0; texelIndex < sourceMip.width * sourceMip.height; ++texelIndex)
		{
			const std::size_t pixelOffset = static_cast<std::size_t>(texelIndex) * 4u;
			mip.data[pixelOffset + 0u] = Pixel::EncodeByteChannel(sourceMip.pixels[pixelOffset + 0u], srgbOutput);
			mip.data[pixelOffset + 1u] = Pixel::EncodeByteChannel(sourceMip.pixels[pixelOffset + 1u], srgbOutput);
			mip.data[pixelOffset + 2u] = Pixel::EncodeByteChannel(sourceMip.pixels[pixelOffset + 2u], srgbOutput);
			mip.data[pixelOffset + 3u] = Pixel::EncodeByteChannel(sourceMip.pixels[pixelOffset + 3u], false);
		}

		return mip;
	}

	static TextureLoadResult BuildUncompressedTexture(const TextureCookRequest& request, const WorkingTexture& workingTexture)
	{
		const DXGI_FORMAT outputFormat = ResolveUncompressedOutputFormat(request, workingTexture.sourceWasFloat);
		TextureLoadResult processedTexture = BuildOutputLayout(workingTexture, outputFormat);

		for (std::size_t arraySliceIndex = 0; arraySliceIndex < workingTexture.arraySlices.size(); ++arraySliceIndex)
		{
			const auto& workingSlice = workingTexture.arraySlices[arraySliceIndex];
			auto& outputSlice = processedTexture.arraySlices[arraySliceIndex];
			outputSlice.reserve(workingSlice.size());

			for (const WorkingMipLevel& workingMip : workingSlice)
			{
				outputSlice.push_back(EncodeUncompressedMip(workingMip, workingTexture.sourceWasFloat, outputFormat));
			}
		}

		return processedTexture;
	}

	static TextureLoadResult BuildCompressedTexture(
	    const TextureCookRequest& request,
	    const WorkingTexture& workingTexture,
	    CompressionTarget target)
	{
		const DXGI_FORMAT outputFormat = ResolveCompressedOutputFormat(request, workingTexture, target);
		const bool srgbOutput = ResolveFormatIntent(outputFormat) == TextureFormatIntent::ColorSrgb;

		BCCompressor compressor(target);
		compressor.Initialize(srgbOutput, HasMeaningfulAlpha(workingTexture));

		TextureLoadResult processedTexture = BuildOutputLayout(workingTexture, outputFormat);

		for (std::size_t arraySliceIndex = 0; arraySliceIndex < workingTexture.arraySlices.size(); ++arraySliceIndex)
		{
			const auto& workingSlice = workingTexture.arraySlices[arraySliceIndex];
			auto& outputSlice = processedTexture.arraySlices[arraySliceIndex];
			outputSlice.reserve(workingSlice.size());

			for (const WorkingMipLevel& workingMip : workingSlice)
			{
				outputSlice.push_back(compressor.CompressMip(request, workingMip));
			}
		}

		return processedTexture;
	}

	TextureLoadResult ProcessCompressedSource(const TextureCookRequest& request, TextureLoadResult sourceTexture)
	{
		if (request.policy.mipPolicy == TextureMipPolicy::Generate && sourceTexture.GetMipCount() == 1)
		{
			throw Diagnostics::Error("Compressed source DDS content has no mip chain and cannot be decompressed for mip generation yet.");
		}

		if (request.policy.dimension == TextureDimension::TextureCube && !sourceTexture.IsCube())
		{
			throw Diagnostics::Error("Compressed source texture does not contain cubemap data.");
		}

		if (request.policy.dimension == TextureDimension::Texture2D && sourceTexture.IsCube())
		{
			throw Diagnostics::Error("Cannot cook cubemap DDS content as a 2D texture.");
		}

		if (request.policy.mipPolicy == TextureMipPolicy::NoMips)
		{
			for (TextureArraySliceData& arraySlice : sourceTexture.arraySlices)
			{
				if (arraySlice.size() > 1)
				{
					arraySlice.resize(1);
				}
			}
		}

		sourceTexture.dxgiFormat = ApplyRequestedColorSpace(sourceTexture.dxgiFormat, request.policy.colorSpace);
		sourceTexture.formatIntent = ResolveFormatIntent(sourceTexture.dxgiFormat);
		return sourceTexture;
	}

	TextureLoadResult BuildOutputTexture(const TextureCookRequest& request, const WorkingTexture& workingTexture)
	{
		const CompressionTarget target = ResolveCompressionTarget(request, workingTexture);
		if (target == CompressionTarget::None)
		{
			return BuildUncompressedTexture(request, workingTexture);
		}

		return BuildCompressedTexture(request, workingTexture, target);
	}
}
