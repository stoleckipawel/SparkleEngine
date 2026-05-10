#include "PCH.h"

#include "Cooking/TextureAssetCooker.h"
#include "Pipeline/TexturePipeline.h"
#include "SourceLoading/TextureSourceLoader.h"

#include "D3D12/Textures/CookedTextureAsset.h"

#include "Core/Public/Diagnostics/Trace.h"
#include "Core/Public/Diagnostics/Logger.h"
#include "Core/Public/Diagnostics/ScopedLogEvent.h"
#include "Core/Public/Files/BinaryStreamWriter.h"
#include "Core/Public/Files/FileUtils.h"

#include <filesystem>
#include <fstream>
#include <limits>

	bool TextureAssetCooker::Cook(const TextureCookRequest& request, std::string& outErrorMessage) const
	{
		static const auto textureCookerLogger = Logging::GetOrCreateLogger("Tools.TextureCooker");
		const std::string scopeName = "Tools.TextureCooker.CookAsset." + request.sourcePath.filename().string();
		SPARKLE_CPU_SCOPE("Tools.TextureCook.Cook");
		SPARKLE_LOG_SCOPE(textureCookerLogger, spdlog::level::info, scopeName);

		if (!request.IsValid())
		{
			outErrorMessage = "Texture cook request is invalid.";
			return false;
		}

		TextureLoadResult loadResult;
		{
			const std::string phaseScopeName = "Tools.TextureCooker.LoadSource." + request.sourcePath.filename().string();
			SPARKLE_CPU_SCOPE(phaseScopeName);
			SPARKLE_LOG_SCOPE(textureCookerLogger, spdlog::level::info, phaseScopeName);
			loadResult = TextureSourceLoader::Load(request.sourcePath, outErrorMessage);
		}
		if (!loadResult.IsValid())
		{
			return false;
		}

		TextureLoadResult cookedTexture;
		{
			const std::string phaseScopeName = "Tools.TextureCooker.ProcessTexture." + request.sourcePath.filename().string();
			SPARKLE_CPU_SCOPE(phaseScopeName);
			SPARKLE_LOG_SCOPE(textureCookerLogger, spdlog::level::info, phaseScopeName);
			if (!TexturePipeline::Process(request, std::move(loadResult), cookedTexture, outErrorMessage))
			{
				return false;
			}
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
		header.dxgiFormat = static_cast<std::uint32_t>(cookedTexture.dxgiFormat);
		header.formatIntent = static_cast<std::uint32_t>(cookedTexture.formatIntent);
		header.mipCount = cookedTexture.GetMipCount();
		header.packedLayout = PackCookedTextureLayout(cookedTexture.dimension, cookedTexture.GetArraySize());

		const std::filesystem::path temporaryOutputPath = Files::BuildTemporaryPath(request.outputPath);
		Files::CleanupTemporaryFile(temporaryOutputPath);

		std::ofstream output;
		{
			const std::string phaseScopeName = "Tools.TextureCooker.WriteTexture." + request.sourcePath.filename().string();
			SPARKLE_CPU_SCOPE(phaseScopeName);
			SPARKLE_LOG_SCOPE(textureCookerLogger, spdlog::level::info, phaseScopeName);
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

		{
			const std::string phaseScopeName = "Tools.TextureCooker.FinalizeTexture." + request.sourcePath.filename().string();
			SPARKLE_CPU_SCOPE(phaseScopeName);
			SPARKLE_LOG_SCOPE(textureCookerLogger, spdlog::level::info, phaseScopeName);
			if (!Files::TryFinalizeTemporaryFile(temporaryOutputPath, request.outputPath, outErrorMessage))
			{
				Files::CleanupTemporaryFile(temporaryOutputPath);
				return false;
			}
		}

		outErrorMessage.clear();
		return true;
	}