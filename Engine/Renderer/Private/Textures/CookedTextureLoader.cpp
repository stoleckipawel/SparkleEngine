#include "PCH.h"

#include "Textures/CookedTextureLoader.h"

#include "Core/Public/Diagnostics/Error.h"
#include "Core/Public/Files/BinarySpanReader.h"
#include "Core/Public/Files/FileUtils.h"
#include "Core/Public/FileSystemUtils.h"
#include "RHI/Public/Textures/CookedTextureAsset.h"

#include <format>
#include <span>

class CookedTextureDecoder final
{
  public:
	static void ValidateHeader(
	    const CookedTextureAssetHeader& header,
	    const std::filesystem::path& resolvedPath)
	{
		if (!header.MatchesExpectedLayout())
			throw Diagnostics::Error("Invalid cooked texture asset header for '" + resolvedPath.string() + "'");

		if (header.width == 0 || header.height == 0 || header.mipCount == 0 ||
		    PixelFormatFromSerializedTextureFormat(header.format) == PixelFormat::Unknown)
		{
			throw Diagnostics::Error(
			    "Cooked texture asset header has invalid dimensions, mip count, or format for '" + resolvedPath.string() + "'");
		}

		if (header.GetArraySize() == 0)
		{
			throw Diagnostics::Error("Cooked texture asset header has an invalid array size for '" + resolvedPath.string() + "'");
		}

		if (header.GetDimension() != TextureResourceDimension::Texture2D &&
		    header.GetDimension() != TextureResourceDimension::TextureCube)
		{
			throw Diagnostics::Error("Cooked texture asset header has an invalid texture dimension for '" + resolvedPath.string() + "'");
		}

		if (header.GetDimension() == TextureResourceDimension::TextureCube && header.GetArraySize() != 6)
		{
			throw Diagnostics::Error("Cooked texture asset header has an invalid cubemap face count for '" + resolvedPath.string() + "'");
		}
	}

	static void ValidateMipHeader(
	    const CookedTextureMipHeader& mipHeader,
	    std::uint32_t mipIndex,
	    const std::filesystem::path& resolvedPath)
	{
		if (mipHeader.width == 0 || mipHeader.height == 0 || mipHeader.rowPitch == 0 ||
		    mipHeader.slicePitch == 0 || mipHeader.dataSize == 0)
		{
			throw Diagnostics::Error(
			    std::format("Cooked texture asset '{}' has an invalid mip header at index {}", resolvedPath.string(), mipIndex));
		}

		if (mipHeader.dataSize != mipHeader.slicePitch)
		{
			throw Diagnostics::Error(std::format(
			    "Cooked texture asset '{}' stores mip {} with {} payload bytes but {} slice pitch bytes",
			    resolvedPath.string(),
			    mipIndex,
			    mipHeader.dataSize,
			    mipHeader.slicePitch));
		}
	}

	static TextureFormatIntent ResolveFormatIntent(std::uint32_t storedValue)
	{
		switch (static_cast<TextureFormatIntent>(storedValue))
		{
			case TextureFormatIntent::Unknown:
			case TextureFormatIntent::ColorSrgb:
			case TextureFormatIntent::DataLinear:
				return static_cast<TextureFormatIntent>(storedValue);
		}

		throw Diagnostics::Error(std::format("Cooked texture stores unknown format intent {}.", storedValue));
	}
};

CookedTextureFilePayload CookedTextureLoader::Read(const std::filesystem::path& texturePath)
{
	CookedTextureFilePayload payload;
	payload.ResolvedPath = Filesystem::ResolveAssetPathValidated(texturePath, AssetType::Texture);
	std::string errorMessage;
	if (!Files::TryReadAllBytes(payload.ResolvedPath, payload.Bytes, errorMessage))
		throw Diagnostics::Error(std::format("Could not read cooked texture '{}': {}", payload.ResolvedPath.string(), errorMessage));
	return payload;
}

LoadedTextureData CookedTextureLoader::Decode(const CookedTextureFilePayload& payload)
{
	if (payload.ResolvedPath.empty() || payload.Bytes.empty())
		throw Diagnostics::Error("Cooked texture decode received an empty file payload.");

	Files::BinarySpanReader reader(payload.Bytes);
	CookedTextureAssetHeader header;
	std::string errorMessage;
	if (!reader.ReadValue(header, errorMessage))
		throw Diagnostics::Error(std::move(errorMessage));
	CookedTextureDecoder::ValidateHeader(header, payload.ResolvedPath);

	const TextureFormatIntent formatIntent = CookedTextureDecoder::ResolveFormatIntent(header.formatIntent);

	std::vector<CookedTextureMipHeader> mipHeaders;
	if (!reader.ReadArray(header.mipCount * header.GetArraySize(), mipHeaders, errorMessage))
		throw Diagnostics::Error(std::move(errorMessage));

	for (std::uint32_t mipIndex = 0; mipIndex < static_cast<std::uint32_t>(mipHeaders.size()); ++mipIndex)
	{
		CookedTextureDecoder::ValidateMipHeader(mipHeaders[mipIndex], mipIndex, payload.ResolvedPath);
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
		if (!reader.ReadBytes(mipHeader.dataSize, mipPayload, errorMessage))
		{
			throw Diagnostics::Error(std::format(
			        "Cooked texture asset '{}' ended before mip payload data could be read.",
			        payload.ResolvedPath.string()));
		}
		mipLevel.Data.assign(mipPayload.begin(), mipPayload.end());

		const std::uint32_t arraySliceIndex = subresourceIndex / mipCount;
		textureUpload.ArraySlices[arraySliceIndex].MipLevels.push_back(std::move(mipLevel));
	}

	if (reader.GetRemainingByteCount() != 0)
	{
		throw Diagnostics::Error(std::format(
		        "Cooked texture asset '{}' contains {} unexpected trailing byte(s)",
		        payload.ResolvedPath.string(),
		        reader.GetRemainingByteCount()));
	}
	if (!textureUpload.IsValid())
		throw Diagnostics::Error(std::format("Cooked texture asset '{}' produced an invalid upload layout.", payload.ResolvedPath.string()));

	return LoadedTextureData{.Upload = std::move(textureUpload), .FormatIntent = formatIntent};
}
