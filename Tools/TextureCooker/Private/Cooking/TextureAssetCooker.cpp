#include "PCH.h"

#include "Cooking/TextureAssetCooker.h"

#include "D3D12/Textures/CookedTextureAsset.h"
#include "D3D12/Textures/TextureLoader.h"

#include <fstream>
#include <limits>
#include <system_error>

namespace Engine::AssetAuthoring
{
	static bool OpenBinaryOutput(const std::filesystem::path& outputPath, std::ofstream& output, std::string& outErrorMessage)
	{
		std::error_code errorCode;
		std::filesystem::create_directories(outputPath.parent_path(), errorCode);
		if (errorCode)
		{
			outErrorMessage = "Failed to create cooked texture output directory '" + outputPath.parent_path().string() + "'";
			return false;
		}

		output.open(outputPath, std::ios::binary | std::ios::trunc);
		if (!output.is_open())
		{
			outErrorMessage = "Failed to open cooked texture output '" + outputPath.string() + "'";
			return false;
		}

		return true;
	}

	static bool WriteBytes(std::ofstream& output, const void* bytes, std::size_t byteCount, std::string& outErrorMessage)
	{
		if (byteCount == 0)
		{
			return true;
		}

		output.write(reinterpret_cast<const char*>(bytes), static_cast<std::streamsize>(byteCount));
		if (output.good())
		{
			return true;
		}

		outErrorMessage = "Failed to write cooked texture asset payload";
		return false;
	}

	bool TextureAssetCooker::Cook(const TextureCookRequest& request, std::string& outErrorMessage) const
	{
		if (!request.IsValid())
		{
			outErrorMessage = "Texture cook request is invalid.";
			return false;
		}

		TextureLoadResult loadResult = TextureLoader::Load(request.sourcePath);
		if (!loadResult.IsValid())
		{
			outErrorMessage = "Failed to load source texture '" + request.sourcePath.string() + "'";
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

		std::ofstream output;
		if (!OpenBinaryOutput(request.outputPath, output, outErrorMessage))
		{
			return false;
		}

		if (!WriteBytes(output, &header, sizeof(header), outErrorMessage) ||
		    !WriteBytes(output, mipHeaders.data(), sizeof(CookedTextureMipHeader) * mipHeaders.size(), outErrorMessage))
		{
			return false;
		}

		for (const TextureMipLevelData& mipLevel : loadResult.mipLevels)
		{
			if (!WriteBytes(output, mipLevel.data.data(), mipLevel.data.size(), outErrorMessage))
			{
				return false;
			}
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