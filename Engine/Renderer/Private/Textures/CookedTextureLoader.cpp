#include "PCH.h"

#include "Textures/CookedTextureLoader.h"

#include "Core/Public/Files/BinarySpanReader.h"
#include "Core/Public/Files/FileUtils.h"
#include "Core/Public/FileSystemUtils.h"
#include "RHI/Public/Textures/CookedTextureAsset.h"

#include <format>
#include <span>

class CookedTextureDecoder final
{
  public:
	static bool ValidateHeader(
	    const CookedTextureAssetHeader& header,
	    const std::filesystem::path& resolvedPath,
	    std::string& outErrorMessage)
	{
		if (!header.MatchesExpectedLayout())
		{
			outErrorMessage = "Invalid cooked texture asset header for '" + resolvedPath.string() + "'";
			return false;
		}

		if (header.width == 0 || header.height == 0 || header.mipCount == 0 ||
		    PixelFormatFromSerializedTextureFormat(header.format) == PixelFormat::Unknown)
		{
			outErrorMessage =
			    "Cooked texture asset header has invalid dimensions, mip count, or format for '" + resolvedPath.string() + "'";
			return false;
		}

		if (header.GetArraySize() == 0)
		{
			outErrorMessage = "Cooked texture asset header has an invalid array size for '" + resolvedPath.string() + "'";
			return false;
		}

		if (header.GetDimension() != TextureResourceDimension::Texture2D &&
		    header.GetDimension() != TextureResourceDimension::TextureCube)
		{
			outErrorMessage = "Cooked texture asset header has an invalid texture dimension for '" + resolvedPath.string() + "'";
			return false;
		}

		if (header.GetDimension() == TextureResourceDimension::TextureCube && header.GetArraySize() != 6)
		{
			outErrorMessage = "Cooked texture asset header has an invalid cubemap face count for '" + resolvedPath.string() + "'";
			return false;
		}

		return true;
	}

	static bool ValidateMipHeader(
	    const CookedTextureMipHeader& mipHeader,
	    std::uint32_t mipIndex,
	    const std::filesystem::path& resolvedPath,
	    std::string& outErrorMessage)
	{
		if (mipHeader.width == 0 || mipHeader.height == 0 || mipHeader.rowPitch == 0 ||
		    mipHeader.slicePitch == 0 || mipHeader.dataSize == 0)
		{
			outErrorMessage =
			    std::format("Cooked texture asset '{}' has an invalid mip header at index {}", resolvedPath.string(), mipIndex);
			return false;
		}

		if (mipHeader.dataSize != mipHeader.slicePitch)
		{
			outErrorMessage = std::format(
			    "Cooked texture asset '{}' stores mip {} with {} payload bytes but {} slice pitch bytes",
			    resolvedPath.string(),
			    mipIndex,
			    mipHeader.dataSize,
			    mipHeader.slicePitch);
			return false;
		}

		return true;
	}

	static bool TryResolveFormatIntent(
	    std::uint32_t storedValue,
	    TextureFormatIntent& outFormatIntent) noexcept
	{
		switch (static_cast<TextureFormatIntent>(storedValue))
		{
			case TextureFormatIntent::Unknown:
			case TextureFormatIntent::ColorSrgb:
			case TextureFormatIntent::DataLinear:
				outFormatIntent = static_cast<TextureFormatIntent>(storedValue);
				return true;
		}

		return false;
	}
};

bool CookedTextureLoader::TryRead(
    const std::filesystem::path& texturePath,
    CookedTextureFilePayload& outPayload,
    std::string& outErrorMessage)
{
	outPayload = {};
	outPayload.ResolvedPath = Filesystem::ResolveAssetPathValidated(texturePath, AssetType::Texture);
	if (!Files::TryReadAllBytes(outPayload.ResolvedPath, outPayload.Bytes, outErrorMessage))
	{
		outPayload = {};
		return false;
	}
	outErrorMessage.clear();
	return true;
}

bool CookedTextureLoader::TryDecode(
    const CookedTextureFilePayload& payload,
    LoadedTextureData& outTexture,
    std::string& outErrorMessage)
{
	outTexture = {};
	if (payload.ResolvedPath.empty() || payload.Bytes.empty())
	{
		outErrorMessage = "Cooked texture decode received an empty file payload.";
		return false;
	}

	Files::BinarySpanReader reader(payload.Bytes);
	CookedTextureAssetHeader header;
	if (!reader.ReadValue(header, outErrorMessage) ||
	    !CookedTextureDecoder::ValidateHeader(header, payload.ResolvedPath, outErrorMessage))
	{
		return false;
	}

	TextureFormatIntent formatIntent = TextureFormatIntent::Unknown;
	if (!CookedTextureDecoder::TryResolveFormatIntent(header.formatIntent, formatIntent))
	{
		outErrorMessage =
		    std::format(
		        "Cooked texture asset '{}' stores an invalid texture format intent {}",
		        payload.ResolvedPath.string(),
		        header.formatIntent);
		return false;
	}

	std::vector<CookedTextureMipHeader> mipHeaders;
	if (!reader.ReadArray(header.mipCount * header.GetArraySize(), mipHeaders, outErrorMessage))
	{
		return false;
	}

	for (std::uint32_t mipIndex = 0; mipIndex < static_cast<std::uint32_t>(mipHeaders.size()); ++mipIndex)
	{
		if (!CookedTextureDecoder::ValidateMipHeader(
		        mipHeaders[mipIndex],
		        mipIndex,
		        payload.ResolvedPath,
		        outErrorMessage))
		{
			return false;
		}
	}

	RhiTextureUploadDesc textureUpload;
	textureUpload.Width = header.width;
	textureUpload.Height = header.height;
	textureUpload.ArraySize = header.GetArraySize();
	textureUpload.Dimension = header.GetDimension();
	textureUpload.Format = PixelFormatFromSerializedTextureFormat(header.format);
	textureUpload.ArraySlices.resize(textureUpload.ArraySize);

	const std::uint32_t mipCount = static_cast<std::uint32_t>(mipHeaders.size() / textureUpload.ArraySize);
	for (std::uint32_t subresourceIndex = 0; subresourceIndex < static_cast<std::uint32_t>(mipHeaders.size()); ++subresourceIndex)
	{
		const CookedTextureMipHeader& mipHeader = mipHeaders[subresourceIndex];
		RhiTextureMipUploadData mipLevel;
		mipLevel.Width = mipHeader.width;
		mipLevel.Height = mipHeader.height;
		mipLevel.RowPitch = mipHeader.rowPitch;
		mipLevel.SlicePitch = mipHeader.slicePitch;

		std::span<const std::uint8_t> mipPayload;
		if (!reader.ReadBytes(mipHeader.dataSize, mipPayload, outErrorMessage))
		{
			outErrorMessage =
			    std::format(
			        "Cooked texture asset '{}' ended before mip payload data could be read.",
			        payload.ResolvedPath.string());
			return false;
		}
		mipLevel.Data.assign(mipPayload.begin(), mipPayload.end());

		const std::uint32_t arraySliceIndex = subresourceIndex / mipCount;
		textureUpload.ArraySlices[arraySliceIndex].MipLevels.push_back(std::move(mipLevel));
	}

	if (reader.GetRemainingByteCount() != 0)
	{
		outErrorMessage =
		    std::format(
		        "Cooked texture asset '{}' contains {} unexpected trailing byte(s)",
		        payload.ResolvedPath.string(),
		        reader.GetRemainingByteCount());
		return false;
	}

	outTexture = LoadedTextureData{.Upload = std::move(textureUpload), .FormatIntent = formatIntent};
	outErrorMessage.clear();
	return true;
}
