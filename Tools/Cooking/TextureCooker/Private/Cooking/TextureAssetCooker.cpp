#include "PCH.h"

#include "Cooking/TextureAssetCooker.h"
#include "Cooking/TextureCookMemoryLimiter.h"
#include "Pipeline/TexturePipeline.h"
#include "SourceLoading/TextureSourceLoader.h"

#include "Textures/CookedTextureAsset.h"

#include "Core/Public/Files/BinaryStreamWriter.h"
#include "Core/Public/Files/FileUtils.h"

#include <filesystem>
#include <fstream>
#include <limits>

class TextureAssetCookerOperations final
{
  public:
	static std::size_t CalculateTextureBytes(const TextureLoadResult& texture)
	{
		std::size_t bytes = 0;
		for (const TextureArraySliceData& slice : texture.arraySlices)
			for (const TextureMipLevelData& mip : slice.mipLevels)
				bytes += mip.data.size();
		return bytes;
	}
};

bool TextureAssetCooker::Cook(
    const TextureCookRequest& request,
    TextureCookMemoryLimiter& memoryLimiter,
    std::stop_token cancellation,
    std::string& outErrorMessage) const
{
	if (!request.IsValid())
	{
		outErrorMessage = "Texture cook request is invalid.";
		return false;
	}

	TextureLoadResult loadResult = TextureSourceLoader::Load(request.sourcePath, outErrorMessage);
	if (!loadResult.IsValid())
	{
		return false;
	}
	auto memoryLease = memoryLimiter.Acquire(TextureAssetCookerOperations::CalculateTextureBytes(loadResult), cancellation);
	if (!memoryLease.IsValid())
	{
		outErrorMessage = "Texture cook was cancelled while waiting for its decompressed-memory budget.";
		return false;
	}

	TextureLoadResult cookedTexture;
	if (!TexturePipeline::Process(request, std::move(loadResult), cookedTexture, outErrorMessage))
	{
		return false;
	}

	if (!cookedTexture.IsValid())
	{
		outErrorMessage = "Texture pipeline produced an invalid cooked texture.";
		return false;
	}

	std::vector<CookedTextureMipHeader> mipHeaders;
	mipHeaders.reserve(cookedTexture.GetSubresourceCount());
	for (const TextureArraySliceData& arraySlice : cookedTexture.arraySlices)
	{
		for (const TextureMipLevelData& mipLevel : arraySlice.mipLevels)
		{
			if (mipLevel.data.size() > (std::numeric_limits<std::uint32_t>::max)())
			{
				outErrorMessage = "Texture mip payload is too large to serialize into a cooked texture asset.";
				return false;
			}

			CookedTextureMipHeader mipHeader;
			mipHeader.width = mipLevel.width;
			mipHeader.height = mipLevel.height;
			mipHeader.rowPitch = mipLevel.rowPitch;
			mipHeader.slicePitch = mipLevel.slicePitch;
			mipHeader.dataSize = static_cast<std::uint32_t>(mipLevel.data.size());
			mipHeaders.push_back(mipHeader);
		}
	}

	CookedTextureAssetHeader header;
	header.magic = kCookedTextureAssetMagic;
	header.version = kCookedTextureAssetVersion;
	header.width = cookedTexture.width;
	header.height = cookedTexture.height;
	header.format = static_cast<std::uint32_t>(cookedTexture.dxgiFormat);
	header.formatIntent = static_cast<std::uint32_t>(cookedTexture.formatIntent);
	header.mipCount = cookedTexture.GetMipCount();
	header.packedLayout = PackCookedTextureLayout(cookedTexture.dimension, cookedTexture.GetArraySize());

	const std::filesystem::path temporaryOutputPath = Files::BuildTemporaryPath(request.outputPath);
	Files::CleanupTemporaryFile(temporaryOutputPath);

	std::ofstream output;
	{
		if (!Files::TryOpenBinaryOutput(temporaryOutputPath, output, outErrorMessage))
		{
			return false;
		}

		if (!Files::BinaryStreamWriter::WriteValue(output, header, outErrorMessage) ||
		    !Files::BinaryStreamWriter::WriteBytes(
		        output,
		        mipHeaders.data(),
		        sizeof(CookedTextureMipHeader) * mipHeaders.size(),
		        outErrorMessage))
		{
			Files::CleanupTemporaryFile(temporaryOutputPath, &output);
			return false;
		}

		for (const TextureArraySliceData& arraySlice : cookedTexture.arraySlices)
		{
			for (const TextureMipLevelData& mipLevel : arraySlice.mipLevels)
			{
				if (!Files::BinaryStreamWriter::WriteBytes(output, mipLevel.data.data(), mipLevel.data.size(), outErrorMessage))
				{
					Files::CleanupTemporaryFile(temporaryOutputPath, &output);
					return false;
				}
			}
		}

		output.flush();
		if (!output.good())
		{
			Files::CleanupTemporaryFile(temporaryOutputPath, &output);
			outErrorMessage = "Failed to flush cooked texture output '" + temporaryOutputPath.string() + "'";
			return false;
		}

		if (!Files::TryCloseOutput(output, temporaryOutputPath, outErrorMessage))
		{
			Files::CleanupTemporaryFile(temporaryOutputPath);
			return false;
		}
	}

	if (!Files::TryFinalizeTemporaryFile(temporaryOutputPath, request.outputPath, outErrorMessage))
	{
		Files::CleanupTemporaryFile(temporaryOutputPath);
		return false;
	}

	outErrorMessage.clear();
	return true;
}
