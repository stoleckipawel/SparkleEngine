#include "PCH.h"

#include "Textures/CookedTextureLoader.h"

#include "Core/Public/Files/BinarySpanReader.h"
#include "Core/Public/Files/FileUtils.h"
#include "Core/Public/FileSystemUtils.h"
#include "RHI/Public/Textures/CookedTextureAsset.h"

#include <format>
#include <span>

static const auto g_cookedTextureLoaderLogger = Logging::GetOrCreateLogger("Renderer.Textures.CookedTextureLoader");

static bool ValidateCookedTextureHeader(
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
		outErrorMessage = "Cooked texture asset header has invalid dimensions, mip count, or format for '" + resolvedPath.string() + "'";
		return false;
	}

	if (header.GetArraySize() == 0)
	{
		outErrorMessage = "Cooked texture asset header has an invalid array size for '" + resolvedPath.string() + "'";
		return false;
	}

	if (header.GetDimension() != TextureResourceDimension::Texture2D && header.GetDimension() != TextureResourceDimension::TextureCube)
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

static bool ValidateCookedTextureMipHeader(
    const CookedTextureMipHeader& mipHeader,
    std::uint32_t mipIndex,
    const std::filesystem::path& resolvedPath,
    std::string& outErrorMessage)
{
	if (mipHeader.width == 0 || mipHeader.height == 0 || mipHeader.rowPitch == 0 || mipHeader.slicePitch == 0 || mipHeader.dataSize == 0)
	{
		outErrorMessage = std::format("Cooked texture asset '{}' has an invalid mip header at index {}", resolvedPath.string(), mipIndex);
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

static bool TryResolveFormatIntent(std::uint32_t storedValue, TextureFormatIntent& outFormatIntent) noexcept
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

RhiTextureUploadDesc CookedTextureLoader::Load(const std::filesystem::path& texturePath)
{
	const std::filesystem::path resolvedPath = Filesystem::ResolveAssetPathValidated(texturePath, AssetType::Texture);
	std::vector<std::uint8_t> fileBytes;
	std::string errorMessage;
	if (!Files::TryReadAllBytes(resolvedPath, fileBytes, errorMessage))
	{
		Diagnostics::Fail(g_cookedTextureLoaderLogger, __FILE__, __LINE__, std::format("CookedTextureLoader: {}", errorMessage));
		return {};
	}

	Files::BinarySpanReader reader(fileBytes);
	CookedTextureAssetHeader header;
	if (!reader.ReadValue(header, errorMessage) || !ValidateCookedTextureHeader(header, resolvedPath, errorMessage))
	{
		Diagnostics::Fail(g_cookedTextureLoaderLogger, __FILE__, __LINE__, std::format("CookedTextureLoader: {}", errorMessage));
		return {};
	}

	TextureFormatIntent formatIntent = TextureFormatIntent::Unknown;
	if (!TryResolveFormatIntent(header.formatIntent, formatIntent))
	{
		Diagnostics::Fail(
		    g_cookedTextureLoaderLogger,
		    __FILE__,
		    __LINE__,
		    std::format("CookedTextureLoader: '{}' stores an invalid texture format intent {}", resolvedPath.string(), header.formatIntent));
		return {};
	}

	std::vector<CookedTextureMipHeader> mipHeaders;
	if (!reader.ReadArray(header.mipCount * header.GetArraySize(), mipHeaders, errorMessage))
	{
		Diagnostics::Fail(g_cookedTextureLoaderLogger, __FILE__, __LINE__, std::format("CookedTextureLoader: {}", errorMessage));
		return {};
	}

	for (std::uint32_t mipIndex = 0; mipIndex < static_cast<std::uint32_t>(mipHeaders.size()); ++mipIndex)
	{
		if (!ValidateCookedTextureMipHeader(mipHeaders[mipIndex], mipIndex, resolvedPath, errorMessage))
		{
			Diagnostics::Fail(g_cookedTextureLoaderLogger, __FILE__, __LINE__, std::format("CookedTextureLoader: {}", errorMessage));
			return {};
		}
	}

	RhiTextureUploadDesc textureUpload;
	textureUpload.Width = header.width;
	textureUpload.Height = header.height;
	textureUpload.ArraySize = header.GetArraySize();
	textureUpload.Dimension = header.GetDimension();
	textureUpload.Format = PixelFormatFromSerializedTextureFormat(header.format);
	textureUpload.FormatIntent = formatIntent;
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
			Diagnostics::Fail(
			    g_cookedTextureLoaderLogger,
			    __FILE__,
			    __LINE__,
			    std::format("CookedTextureLoader: '{}' ended before mip payload data could be read.", resolvedPath.string()));
			return {};
		}
		mipLevel.Data.assign(mipPayload.begin(), mipPayload.end());

		const std::uint32_t arraySliceIndex = subresourceIndex / mipCount;
		textureUpload.ArraySlices[arraySliceIndex].MipLevels.push_back(std::move(mipLevel));
	}

	if (reader.GetRemainingByteCount() != 0)
	{
		Diagnostics::Fail(
		    g_cookedTextureLoaderLogger,
		    __FILE__,
		    __LINE__,
		    std::format(
		        "CookedTextureLoader: '{}' contains {} unexpected trailing byte(s)",
		        resolvedPath.string(),
		        reader.GetRemainingByteCount()));
		return {};
	}

	return textureUpload;
}