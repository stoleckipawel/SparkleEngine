#include "PCH.h"

#include "Cooking/TextureAssetCooker.h"
#include "SourceLoading/TextureSourceLoader.h"

#include "D3D12/Textures/CookedTextureAsset.h"

#include "Core/Public/Diagnostics/Trace.h"
#include "Core/Public/Files/BinaryStreamWriter.h"
#include "Core/Public/Files/FileUtils.h"

#include <filesystem>
#include <fstream>
#include <limits>

namespace AssetAuthoring
{
	bool TextureAssetCooker::Cook(const TextureCookRequest& request, std::string& outErrorMessage) const
	{
		SPARKLE_CPU_SCOPE("Tools.TextureCook.Cook");

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

		DXGI_FORMAT cookedDxgiFormat = DXGI_FORMAT_UNKNOWN;
		TextureFormatIntent formatIntent = TextureFormatIntent::Unknown;
		if (!ResolveCookedTextureFormat(loadResult.dxgiFormat, request.colorSpace, cookedDxgiFormat, formatIntent, outErrorMessage))
		{
			return false;
		}

		std::vector<CookedTextureMipHeader> mipHeaders;
		mipHeaders.reserve(loadResult.mipLevels.size());
		for (const TextureMipLevelData& mipLevel : loadResult.mipLevels)
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

		CookedTextureAssetHeader header;
		header.magic = kCookedTextureAssetMagic;
		header.version = kCookedTextureAssetVersion;
		header.width = loadResult.width;
		header.height = loadResult.height;
		header.dxgiFormat = static_cast<std::uint32_t>(cookedDxgiFormat);
		header.formatIntent = static_cast<std::uint32_t>(formatIntent);
		header.mipCount = static_cast<std::uint32_t>(mipHeaders.size());

		const std::filesystem::path temporaryOutputPath = Files::BuildTemporaryPath(request.outputPath);
		Files::CleanupTemporaryFile(temporaryOutputPath);

		std::ofstream output;
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

		for (const TextureMipLevelData& mipLevel : loadResult.mipLevels)
		{
			if (!Files::BinaryStreamWriter::WriteBytes(output, mipLevel.data.data(), mipLevel.data.size(), outErrorMessage))
			{
				Files::CleanupTemporaryFile(temporaryOutputPath, &output);
				return false;
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

		if (!Files::TryFinalizeTemporaryFile(temporaryOutputPath, request.outputPath, outErrorMessage))
		{
			Files::CleanupTemporaryFile(temporaryOutputPath);
			return false;
		}

		outErrorMessage.clear();
		return true;
	}

	bool TextureAssetCooker::ResolveCookedTextureFormat(
	    DXGI_FORMAT sourceDxgiFormat,
	    TextureColorSpace colorSpace,
	    DXGI_FORMAT& outCookedDxgiFormat,
	    TextureFormatIntent& outFormatIntent,
	    std::string& outErrorMessage)
	{
		const bool wantsSrgb = colorSpace == TextureColorSpace::Srgb;

		switch (sourceDxgiFormat)
		{
			case DXGI_FORMAT_R8G8B8A8_UNORM:
			case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
				outCookedDxgiFormat = wantsSrgb ? DXGI_FORMAT_R8G8B8A8_UNORM_SRGB : DXGI_FORMAT_R8G8B8A8_UNORM;
				outFormatIntent = wantsSrgb ? TextureFormatIntent::ColorSrgb : TextureFormatIntent::DataLinear;
				outErrorMessage.clear();
				return true;

			case DXGI_FORMAT_B8G8R8A8_UNORM:
			case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
				outCookedDxgiFormat = wantsSrgb ? DXGI_FORMAT_B8G8R8A8_UNORM_SRGB : DXGI_FORMAT_B8G8R8A8_UNORM;
				outFormatIntent = wantsSrgb ? TextureFormatIntent::ColorSrgb : TextureFormatIntent::DataLinear;
				outErrorMessage.clear();
				return true;

			case DXGI_FORMAT_BC1_UNORM:
			case DXGI_FORMAT_BC1_UNORM_SRGB:
				outCookedDxgiFormat = wantsSrgb ? DXGI_FORMAT_BC1_UNORM_SRGB : DXGI_FORMAT_BC1_UNORM;
				outFormatIntent = wantsSrgb ? TextureFormatIntent::ColorSrgb : TextureFormatIntent::DataLinear;
				outErrorMessage.clear();
				return true;

			case DXGI_FORMAT_BC2_UNORM:
			case DXGI_FORMAT_BC2_UNORM_SRGB:
				outCookedDxgiFormat = wantsSrgb ? DXGI_FORMAT_BC2_UNORM_SRGB : DXGI_FORMAT_BC2_UNORM;
				outFormatIntent = wantsSrgb ? TextureFormatIntent::ColorSrgb : TextureFormatIntent::DataLinear;
				outErrorMessage.clear();
				return true;

			case DXGI_FORMAT_BC3_UNORM:
			case DXGI_FORMAT_BC3_UNORM_SRGB:
				outCookedDxgiFormat = wantsSrgb ? DXGI_FORMAT_BC3_UNORM_SRGB : DXGI_FORMAT_BC3_UNORM;
				outFormatIntent = wantsSrgb ? TextureFormatIntent::ColorSrgb : TextureFormatIntent::DataLinear;
				outErrorMessage.clear();
				return true;

			case DXGI_FORMAT_BC4_UNORM:
			case DXGI_FORMAT_BC4_SNORM:
			case DXGI_FORMAT_BC5_UNORM:
			case DXGI_FORMAT_BC5_SNORM:
			case DXGI_FORMAT_R16G16B16A16_FLOAT:
			case DXGI_FORMAT_R32G32B32A32_FLOAT:
				if (wantsSrgb)
				{
					outErrorMessage = "Requested sRGB cooking for a data texture format that cannot be interpreted as sRGB.";
					return false;
				}

				outCookedDxgiFormat = sourceDxgiFormat;
				outFormatIntent = TextureFormatIntent::DataLinear;
				outErrorMessage.clear();
				return true;

			default:
				outErrorMessage = "Unsupported source texture DXGI format for cooked texture emission.";
				return false;
		}
	}
}