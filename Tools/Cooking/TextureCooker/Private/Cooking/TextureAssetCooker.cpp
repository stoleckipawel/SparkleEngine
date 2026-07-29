#include "PCH.h"

#include "Cooking/TextureAssetCooker.h"
#include "Cooking/TextureCookMemoryLimiter.h"
#include "Pipeline/TexturePipeline.h"
#include "SourceLoading/TextureSourceLoader.h"

#include "Textures/CookedTextureAsset.h"

#include "Core/Public/Diagnostics/Error.h"
#include "Core/Public/Files/BinaryStreamWriter.h"
#include "Core/Public/Files/FileUtils.h"

#include <filesystem>
#include <fstream>
#include <limits>

class TextureMemoryEstimator final
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

void TextureAssetCooker::Cook(
    const TextureCookRequest& request,
    TextureCookMemoryLimiter& memoryLimiter,
    std::stop_token cancellation) const
{
	ValidateTextureCookRequest(request);

	TextureLoadResult loadResult = TextureSourceLoader::Load(request.sourcePath);
	auto memoryLease = memoryLimiter.Acquire(TextureMemoryEstimator::CalculateTextureBytes(loadResult), cancellation);
	TextureLoadResult cookedTexture = TexturePipeline::Process(request, std::move(loadResult));

	std::vector<CookedTextureMipHeader> mipHeaders;
	mipHeaders.reserve(cookedTexture.GetSubresourceCount());
	for (const TextureArraySliceData& arraySlice : cookedTexture.arraySlices)
	{
		for (const TextureMipLevelData& mipLevel : arraySlice.mipLevels)
		{
			if (mipLevel.data.size() > (std::numeric_limits<std::uint32_t>::max)())
			{
				throw Diagnostics::Error("Texture mip payload is too large to serialize into a cooked texture asset.");
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
	std::string fileError;
	try
	{
		if (!Files::TryOpenBinaryOutput(temporaryOutputPath, output, fileError))
		{
			throw Diagnostics::Error(std::move(fileError));
		}

		if (!Files::BinaryStreamWriter::WriteValue(output, header, fileError) ||
		    !Files::BinaryStreamWriter::WriteBytes(
		        output,
		        mipHeaders.data(),
		        sizeof(CookedTextureMipHeader) * mipHeaders.size(),
		        fileError))
		{
			throw Diagnostics::Error(std::move(fileError));
		}

		for (const TextureArraySliceData& arraySlice : cookedTexture.arraySlices)
		{
			for (const TextureMipLevelData& mipLevel : arraySlice.mipLevels)
			{
				if (!Files::BinaryStreamWriter::WriteBytes(output, mipLevel.data.data(), mipLevel.data.size(), fileError))
				{
					throw Diagnostics::Error(std::move(fileError));
				}
			}
		}

		output.flush();
		if (!output.good())
		{
			throw Diagnostics::Error("Failed to flush cooked texture output '" + temporaryOutputPath.string() + "'.");
		}

		if (!Files::TryCloseOutput(output, temporaryOutputPath, fileError))
		{
			throw Diagnostics::Error(std::move(fileError));
		}

		if (!Files::TryFinalizeTemporaryFile(temporaryOutputPath, request.outputPath, fileError))
		{
			throw Diagnostics::Error(std::move(fileError));
		}
	}
	catch (...)
	{
		Files::CleanupTemporaryFile(temporaryOutputPath, &output);
		throw;
	}
}
