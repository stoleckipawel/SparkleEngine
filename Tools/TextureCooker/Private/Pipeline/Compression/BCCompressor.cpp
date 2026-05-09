#include "PCH.h"

#include "Pipeline/Compression/BCCompressor.h"

#include "Core/Public/Pixel/FloatConversion.h"
#include "Core/Public/Pixel/PixelFormat.h"

#include <cmp_core.h>

#include <algorithm>
#include <array>

namespace TextureCookPipeline
{
	static constexpr float kBlockCompressionQuality = 1.0f;

	BCCompressor::BCCompressor(CompressionTarget target) noexcept : target_(target) {}

	BCCompressor::~BCCompressor()
	{
		Destroy();
	}

	bool BCCompressor::Initialize(bool srgbOutput, bool imageNeedsAlpha, std::string& outErrorMessage)
	{
		Destroy();

		int result = 0;
		switch (target_)
		{
			case CompressionTarget::BC1:
				result = CreateOptionsBC1(&options_);
				if (result == 0)
				{
					SetQualityBC1(options_, 1.0f);
				}
				break;
			case CompressionTarget::BC4:
				result = CreateOptionsBC4(&options_);
				if (result == 0)
				{
					SetQualityBC4(options_, 1.0f);
				}
				break;
			case CompressionTarget::BC5:
				result = CreateOptionsBC5(&options_);
				if (result == 0)
				{
					SetQualityBC5(options_, 1.0f);
				}
				break;
			case CompressionTarget::BC6H:
				result = CreateOptionsBC6(&options_);
				if (result == 0)
				{
					SetQualityBC6(options_, kBlockCompressionQuality);
					SetSignedBC6(options_, false);
				}
				break;
			case CompressionTarget::BC7:
				result = CreateOptionsBC7(&options_);
				if (result == 0)
				{
					SetQualityBC7(options_, kBlockCompressionQuality);
					SetMaskBC7(options_, 0xffu);
					SetAlphaOptionsBC7(options_, imageNeedsAlpha, false, false);
				}
				break;
			case CompressionTarget::None:
				outErrorMessage = "Cannot initialize a BC compressor without a compression target.";
				return false;
		}

		if (result != 0)
		{
			outErrorMessage = "Failed to initialize CMP_Core compression options.";
			return false;
		}

		if (srgbOutput && target_ == CompressionTarget::BC1)
		{
			SetSrgbBC1(options_, false);
		}

		outErrorMessage.clear();
		return true;
	}

	bool BCCompressor::CompressMip(
	    const TextureCookRequest& request,
	    const WorkingMipLevel& sourceMip,
	    TextureMipLevelData& outMip,
	    std::string& outErrorMessage) const
	{
		outMip.width = sourceMip.width;
		outMip.height = sourceMip.height;
		outMip.rowPitch = ComputeBlockCompressedRowPitch(target_, sourceMip.width);
		outMip.slicePitch = ComputeBlockCompressedSlicePitch(target_, sourceMip.width, sourceMip.height);
		outMip.data.resize(outMip.slicePitch);

		const std::uint32_t blockCountX = (std::max)(1u, (sourceMip.width + 3u) / 4u);
		const std::uint32_t blockCountY = (std::max)(1u, (sourceMip.height + 3u) / 4u);
		const bool srgbOutput = request.colorSpace == TextureColorSpace::Srgb;

		for (std::uint32_t blockY = 0; blockY < blockCountY; ++blockY)
		{
			for (std::uint32_t blockX = 0; blockX < blockCountX; ++blockX)
			{
				std::uint8_t* destinationBlock = outMip.data.data() + (static_cast<std::size_t>(blockY) * outMip.rowPitch) +
				                                 (static_cast<std::size_t>(blockX) *
				                                  (target_ == CompressionTarget::BC1 || target_ == CompressionTarget::BC4 ? 8u : 16u));

				if (target_ == CompressionTarget::BC1 || target_ == CompressionTarget::BC7)
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
							rgbaBlock[blockOffset + 0u] = Pixel::EncodeByteChannel(sourceMip.pixels[sourceOffset + 0u], srgbOutput);
							rgbaBlock[blockOffset + 1u] = Pixel::EncodeByteChannel(sourceMip.pixels[sourceOffset + 1u], srgbOutput);
							rgbaBlock[blockOffset + 2u] = Pixel::EncodeByteChannel(sourceMip.pixels[sourceOffset + 2u], srgbOutput);
							rgbaBlock[blockOffset + 3u] = Pixel::EncodeByteChannel(sourceMip.pixels[sourceOffset + 3u], false);
						}
					}

					const int result = target_ == CompressionTarget::BC1
					                       ? CompressBlockBC1(rgbaBlock.data(), 16u, destinationBlock, options_)
					                       : CompressBlockBC7(rgbaBlock.data(), 16u, destinationBlock, options_);
					if (result != 0)
					{
						outErrorMessage = "CMP_Core failed to compress an RGBA block.";
						return false;
					}

					continue;
				}

				if (target_ == CompressionTarget::BC4)
				{
					std::array<std::uint8_t, 16> block{};
					for (std::uint32_t localY = 0; localY < 4u; ++localY)
					{
						for (std::uint32_t localX = 0; localX < 4u; ++localX)
						{
							const std::uint32_t sampleX = (std::min)(sourceMip.width - 1u, (blockX * 4u) + localX);
							const std::uint32_t sampleY = (std::min)(sourceMip.height - 1u, (blockY * 4u) + localY);
							const std::size_t sourceOffset = (static_cast<std::size_t>(sampleY) * sourceMip.width + sampleX) * 4u;
							block[(localY * 4u) + localX] = Pixel::EncodeByteChannel(sourceMip.pixels[sourceOffset + 0u], false);
						}
					}

					if (CompressBlockBC4(block.data(), 4u, destinationBlock, options_) != 0)
					{
						outErrorMessage = "CMP_Core failed to compress a BC4 block.";
						return false;
					}

					continue;
				}

				if (target_ == CompressionTarget::BC5)
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
							blockRed[(localY * 4u) + localX] = Pixel::EncodeByteChannel(sourceMip.pixels[sourceOffset + 0u], false);
							blockGreen[(localY * 4u) + localX] = Pixel::EncodeByteChannel(sourceMip.pixels[sourceOffset + 1u], false);
						}
					}

					if (CompressBlockBC5(blockRed.data(), 4u, blockGreen.data(), 4u, destinationBlock, options_) != 0)
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
						blockHalf[blockOffset + 0u] = Pixel::FloatToHalf((std::max)(0.0f, sourceMip.pixels[sourceOffset + 0u]));
						blockHalf[blockOffset + 1u] = Pixel::FloatToHalf((std::max)(0.0f, sourceMip.pixels[sourceOffset + 1u]));
						blockHalf[blockOffset + 2u] = Pixel::FloatToHalf((std::max)(0.0f, sourceMip.pixels[sourceOffset + 2u]));
					}
				}

				if (CompressBlockBC6(blockHalf.data(), 12u, destinationBlock, options_) != 0)
				{
					outErrorMessage = "CMP_Core failed to compress a BC6H block.";
					return false;
				}
			}
		}

		outErrorMessage.clear();
		return true;
	}

	void BCCompressor::Destroy() noexcept
	{
		if (options_ == nullptr)
		{
			return;
		}

		switch (target_)
		{
			case CompressionTarget::BC1:
				DestroyOptionsBC1(options_);
				break;
			case CompressionTarget::BC4:
				DestroyOptionsBC4(options_);
				break;
			case CompressionTarget::BC5:
				DestroyOptionsBC5(options_);
				break;
			case CompressionTarget::BC6H:
				DestroyOptionsBC6(options_);
				break;
			case CompressionTarget::BC7:
				DestroyOptionsBC7(options_);
				break;
			case CompressionTarget::None:
				break;
		}

		options_ = nullptr;
	}
}