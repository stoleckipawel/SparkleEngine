#include "PCH.h"

#include "Cooking/Pipeline/TextureCookOutputStage.h"

#include "Cooking/Pipeline/TextureCookPipelineUtils.h"

#include <cmp_core.h>

#include <algorithm>
#include <array>
#include <cstring>

namespace TextureCookPipeline
{
	namespace
	{
		CompressionTarget ResolveCompressionTarget(const TextureCookRequest& request, const WorkingImage& workingImage) noexcept
		{
			switch (request.compressionFamilyPreference)
			{
				case TextureCompressionFamilyPreference::None:
					return CompressionTarget::None;
				case TextureCompressionFamilyPreference::NormalMap:
					return CompressionTarget::BC5;
				case TextureCompressionFamilyPreference::HdrColor:
					return CompressionTarget::BC6H;
				case TextureCompressionFamilyPreference::Masks:
					return IsGreyscaleLike(workingImage) ? CompressionTarget::BC4 : CompressionTarget::BC7;
				case TextureCompressionFamilyPreference::CubeColor:
					if (workingImage.sourceWasFloat)
					{
						return CompressionTarget::BC6H;
					}

					return HasMeaningfulAlpha(workingImage) ? CompressionTarget::BC7 : CompressionTarget::BC1;
				case TextureCompressionFamilyPreference::Color:
				default:
					if (workingImage.sourceWasFloat)
					{
						return CompressionTarget::BC6H;
					}

					return HasMeaningfulAlpha(workingImage) ? CompressionTarget::BC7 : CompressionTarget::BC1;
			}
		}

		void* CreateCompressionOptions(CompressionTarget target, bool srgbOutput, bool imageNeedsAlpha, std::string& outErrorMessage)
		{
			void* options = nullptr;
			int result = 0;

			switch (target)
			{
				case CompressionTarget::BC1:
					result = CreateOptionsBC1(&options);
					if (result == 0)
					{
						SetQualityBC1(options, 1.0f);
					}
					break;
				case CompressionTarget::BC4:
					result = CreateOptionsBC4(&options);
					if (result == 0)
					{
						SetQualityBC4(options, 1.0f);
					}
					break;
				case CompressionTarget::BC5:
					result = CreateOptionsBC5(&options);
					if (result == 0)
					{
						SetQualityBC5(options, 1.0f);
					}
					break;
				case CompressionTarget::BC6H:
					result = CreateOptionsBC6(&options);
					if (result == 0)
					{
						SetQualityBC6(options, 1.0f);
						SetSignedBC6(options, false);
					}
					break;
				case CompressionTarget::BC7:
					result = CreateOptionsBC7(&options);
					if (result == 0)
					{
						SetQualityBC7(options, 1.0f);
						SetMaskBC7(options, 0xffu);
						SetAlphaOptionsBC7(options, imageNeedsAlpha, false, false);
					}
					break;
				case CompressionTarget::None:
					outErrorMessage.clear();
					return nullptr;
			}

			if (result != 0)
			{
				outErrorMessage = "Failed to initialize CMP_Core compression options.";
				return nullptr;
			}

			if (srgbOutput && target == CompressionTarget::BC1)
			{
				SetSrgbBC1(options, false);
			}

			outErrorMessage.clear();
			return options;
		}

		void DestroyCompressionOptions(CompressionTarget target, void* compressionOptions) noexcept
		{
			if (compressionOptions == nullptr)
			{
				return;
			}

			switch (target)
			{
				case CompressionTarget::BC1:
					DestroyOptionsBC1(compressionOptions);
					return;
				case CompressionTarget::BC4:
					DestroyOptionsBC4(compressionOptions);
					return;
				case CompressionTarget::BC5:
					DestroyOptionsBC5(compressionOptions);
					return;
				case CompressionTarget::BC6H:
					DestroyOptionsBC6(compressionOptions);
					return;
				case CompressionTarget::BC7:
					DestroyOptionsBC7(compressionOptions);
					return;
				case CompressionTarget::None:
					return;
			}
		}

		bool EncodeUncompressedMip(
		    const TextureCookRequest& request,
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
				outMip.data[pixelOffset + 0u] = EncodeByteChannel(sourceMip.pixels[pixelOffset + 0u], srgbOutput);
				outMip.data[pixelOffset + 1u] = EncodeByteChannel(sourceMip.pixels[pixelOffset + 1u], srgbOutput);
				outMip.data[pixelOffset + 2u] = EncodeByteChannel(sourceMip.pixels[pixelOffset + 2u], srgbOutput);
				outMip.data[pixelOffset + 3u] = EncodeByteChannel(sourceMip.pixels[pixelOffset + 3u], false);
			}

			outErrorMessage.clear();
			return true;
		}

		bool CompressMipLevel(
		    const TextureCookRequest& request,
		    const WorkingMipLevel& sourceMip,
		    CompressionTarget target,
		    void* compressionOptions,
		    TextureMipLevelData& outMip,
		    std::string& outErrorMessage)
		{
			outMip.width = sourceMip.width;
			outMip.height = sourceMip.height;
			outMip.rowPitch = ComputeBlockCompressedRowPitch(target, sourceMip.width);
			outMip.slicePitch = ComputeBlockCompressedSlicePitch(target, sourceMip.width, sourceMip.height);
			outMip.data.resize(outMip.slicePitch);

			const std::uint32_t blockCountX = (std::max)(1u, (sourceMip.width + 3u) / 4u);
			const std::uint32_t blockCountY = (std::max)(1u, (sourceMip.height + 3u) / 4u);
			const bool srgbOutput = request.colorSpace == TextureColorSpace::Srgb;

			for (std::uint32_t blockY = 0; blockY < blockCountY; ++blockY)
			{
				for (std::uint32_t blockX = 0; blockX < blockCountX; ++blockX)
				{
					std::uint8_t* destinationBlock = outMip.data.data() + (static_cast<std::size_t>(blockY) * outMip.rowPitch) +
					                               (static_cast<std::size_t>(blockX) * (target == CompressionTarget::BC1 || target == CompressionTarget::BC4 ? 8u : 16u));

					if (target == CompressionTarget::BC1 || target == CompressionTarget::BC7)
					{
						std::array<std::uint8_t, 64> rgbaBlock{};
						for (std::uint32_t localY = 0; localY < 4u; ++localY)
						{
							for (std::uint32_t localX = 0; localX < 4u; ++localX)
							{
								const std::uint32_t sampleX = (std::min)(sourceMip.width - 1u, (blockX * 4u) + localX);
								const std::uint32_t sampleY = (std::min)(sourceMip.height - 1u, (blockY * 4u) + localY);
								const std::size_t sourceOffset = (static_cast<std::size_t>(sampleY) * sourceMip.width + sampleX) * 4u;
								const std::size_t blockOffset = (static_cast<std::size_t>(localY) * 4u + localX) * 4u;
								rgbaBlock[blockOffset + 0u] = EncodeByteChannel(sourceMip.pixels[sourceOffset + 0u], srgbOutput);
								rgbaBlock[blockOffset + 1u] = EncodeByteChannel(sourceMip.pixels[sourceOffset + 1u], srgbOutput);
								rgbaBlock[blockOffset + 2u] = EncodeByteChannel(sourceMip.pixels[sourceOffset + 2u], srgbOutput);
								rgbaBlock[blockOffset + 3u] = EncodeByteChannel(sourceMip.pixels[sourceOffset + 3u], false);
							}
						}

						const int result = target == CompressionTarget::BC1 ? CompressBlockBC1(rgbaBlock.data(), 16u, destinationBlock, compressionOptions)
						                                                   : CompressBlockBC7(rgbaBlock.data(), 16u, destinationBlock, compressionOptions);
						if (result != 0)
						{
							outErrorMessage = "CMP_Core failed to compress an RGBA block.";
							return false;
						}

						continue;
					}

					if (target == CompressionTarget::BC4)
					{
						std::array<std::uint8_t, 16> block{};
						for (std::uint32_t localY = 0; localY < 4u; ++localY)
						{
							for (std::uint32_t localX = 0; localX < 4u; ++localX)
							{
								const std::uint32_t sampleX = (std::min)(sourceMip.width - 1u, (blockX * 4u) + localX);
								const std::uint32_t sampleY = (std::min)(sourceMip.height - 1u, (blockY * 4u) + localY);
								const std::size_t sourceOffset = (static_cast<std::size_t>(sampleY) * sourceMip.width + sampleX) * 4u;
								block[(localY * 4u) + localX] = EncodeByteChannel(sourceMip.pixels[sourceOffset + 0u], false);
							}
						}

						if (CompressBlockBC4(block.data(), 4u, destinationBlock, compressionOptions) != 0)
						{
							outErrorMessage = "CMP_Core failed to compress a BC4 block.";
							return false;
						}

						continue;
					}

					if (target == CompressionTarget::BC5)
					{
						std::array<std::uint8_t, 16> blockRed{};
						std::array<std::uint8_t, 16> blockGreen{};
						for (std::uint32_t localY = 0; localY < 4u; ++localY)
						{
							for (std::uint32_t localX = 0; localX < 4u; ++localX)
							{
								const std::uint32_t sampleX = (std::min)(sourceMip.width - 1u, (blockX * 4u) + localX);
								const std::uint32_t sampleY = (std::min)(sourceMip.height - 1u, (blockY * 4u) + localY);
								const std::size_t sourceOffset = (static_cast<std::size_t>(sampleY) * sourceMip.width + sampleX) * 4u;
								blockRed[(localY * 4u) + localX] = EncodeByteChannel(sourceMip.pixels[sourceOffset + 0u], false);
								blockGreen[(localY * 4u) + localX] = EncodeByteChannel(sourceMip.pixels[sourceOffset + 1u], false);
							}
						}

						if (CompressBlockBC5(blockRed.data(), 4u, blockGreen.data(), 4u, destinationBlock, compressionOptions) != 0)
						{
							outErrorMessage = "CMP_Core failed to compress a BC5 block.";
							return false;
						}

						continue;
					}

					std::array<std::uint16_t, 48> blockHalf{};
					for (std::uint32_t localY = 0; localY < 4u; ++localY)
					{
						for (std::uint32_t localX = 0; localX < 4u; ++localX)
						{
							const std::uint32_t sampleX = (std::min)(sourceMip.width - 1u, (blockX * 4u) + localX);
							const std::uint32_t sampleY = (std::min)(sourceMip.height - 1u, (blockY * 4u) + localY);
							const std::size_t sourceOffset = (static_cast<std::size_t>(sampleY) * sourceMip.width + sampleX) * 4u;
							const std::size_t blockOffset = (static_cast<std::size_t>(localY) * 12u) + (localX * 3u);
							blockHalf[blockOffset + 0u] = FloatToHalf((std::max)(0.0f, sourceMip.pixels[sourceOffset + 0u]));
							blockHalf[blockOffset + 1u] = FloatToHalf((std::max)(0.0f, sourceMip.pixels[sourceOffset + 1u]));
							blockHalf[blockOffset + 2u] = FloatToHalf((std::max)(0.0f, sourceMip.pixels[sourceOffset + 2u]));
						}
					}

					if (CompressBlockBC6(blockHalf.data(), 12u, destinationBlock, compressionOptions) != 0)
					{
						outErrorMessage = "CMP_Core failed to compress a BC6H block.";
						return false;
					}
				}
			}

			outErrorMessage.clear();
			return true;
		}

		bool BuildUncompressedTexture(
		    const TextureCookRequest& request,
		    const WorkingImage& workingImage,
		    TextureLoadResult& outProcessedTexture,
		    std::string& outErrorMessage)
		{
			const DXGI_FORMAT outputFormat = ResolveUncompressedOutputFormat(request, workingImage.sourceWasFloat);
			outProcessedTexture = {};
			outProcessedTexture.width = workingImage.arraySlices.front().front().width;
			outProcessedTexture.height = workingImage.arraySlices.front().front().height;
			outProcessedTexture.arraySize = workingImage.arraySize;
			outProcessedTexture.dimension = workingImage.dimension;
			outProcessedTexture.dxgiFormat = outputFormat;
			outProcessedTexture.formatIntent = ResolveFormatIntent(outputFormat);
			outProcessedTexture.arraySlices.resize(workingImage.arraySlices.size());

			for (std::size_t arraySliceIndex = 0; arraySliceIndex < workingImage.arraySlices.size(); ++arraySliceIndex)
			{
				const auto& workingSlice = workingImage.arraySlices[arraySliceIndex];
				auto& outputSlice = outProcessedTexture.arraySlices[arraySliceIndex];
				outputSlice.mipLevels.reserve(workingSlice.size());

				for (const WorkingMipLevel& workingMip : workingSlice)
				{
					TextureMipLevelData outputMip;
					if (!EncodeUncompressedMip(request, workingMip, workingImage.sourceWasFloat, outputFormat, outputMip, outErrorMessage))
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
		    const WorkingImage& workingImage,
		    CompressionTarget target,
		    TextureLoadResult& outProcessedTexture,
		    std::string& outErrorMessage)
		{
			const DXGI_FORMAT outputFormat = ResolveCompressedOutputFormat(request, workingImage, target);
			const bool srgbOutput = ResolveFormatIntent(outputFormat) == TextureFormatIntent::ColorSrgb;
			void* compressionOptions = CreateCompressionOptions(target, srgbOutput, HasMeaningfulAlpha(workingImage), outErrorMessage);
			if (!outErrorMessage.empty())
			{
				return false;
			}

			outProcessedTexture = {};
			outProcessedTexture.width = workingImage.arraySlices.front().front().width;
			outProcessedTexture.height = workingImage.arraySlices.front().front().height;
			outProcessedTexture.arraySize = workingImage.arraySize;
			outProcessedTexture.dimension = workingImage.dimension;
			outProcessedTexture.dxgiFormat = outputFormat;
			outProcessedTexture.formatIntent = ResolveFormatIntent(outputFormat);
			outProcessedTexture.arraySlices.resize(workingImage.arraySlices.size());

			for (std::size_t arraySliceIndex = 0; arraySliceIndex < workingImage.arraySlices.size(); ++arraySliceIndex)
			{
				const auto& workingSlice = workingImage.arraySlices[arraySliceIndex];
				auto& outputSlice = outProcessedTexture.arraySlices[arraySliceIndex];
				outputSlice.mipLevels.reserve(workingSlice.size());

				for (const WorkingMipLevel& workingMip : workingSlice)
				{
					TextureMipLevelData outputMip;
					if (!CompressMipLevel(request, workingMip, target, compressionOptions, outputMip, outErrorMessage))
					{
						DestroyCompressionOptions(target, compressionOptions);
						return false;
					}

					outputSlice.mipLevels.push_back(std::move(outputMip));
				}
			}

			DestroyCompressionOptions(target, compressionOptions);
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
	    const WorkingImage& workingImage,
	    TextureLoadResult& outProcessedTexture,
	    std::string& outErrorMessage)
	{
		const CompressionTarget target = ResolveCompressionTarget(request, workingImage);
		if (target == CompressionTarget::None)
		{
			return BuildUncompressedTexture(request, workingImage, outProcessedTexture, outErrorMessage);
		}

		return BuildCompressedTexture(request, workingImage, target, outProcessedTexture, outErrorMessage);
	}
}
