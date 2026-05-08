#include "PCH.h"

#include "D3D12/Textures/CookedTextureAssetLoader.h"

#include "Core/Public/Files/FileUtils.h"
#include "Core/Public/FileSystemUtils.h"

#include <format>
#include <span>

static const auto g_cookedTextureAssetLoaderLogger = Logging::GetOrCreateLogger("RHI.Textures");

bool CookedTextureAssetLoader::SupportsExtension(std::wstring_view extension) const noexcept
{
	return extension == kCookedTextureAssetExtensionWide;
}

TextureLoadResult CookedTextureAssetLoader::Load(const std::filesystem::path& fileName) const
{
	const std::filesystem::path resolvedPath = Filesystem::ResolveAssetPathValidated(fileName, AssetType::Texture);
	std::vector<std::uint8_t> fileBytes;
	std::string errorMessage;
	if (!Files::TryReadAllBytes(resolvedPath, fileBytes, errorMessage))
	{
		Diagnostics::Fail(
		    g_cookedTextureAssetLoaderLogger,
		    __FILE__,
		    __LINE__,
		    std::format("CookedTextureAssetLoader: {}", errorMessage));
		return {};
	}

	Files::BinarySpanReader reader(fileBytes);
	CookedTextureAssetHeader header;
	if (!reader.ReadValue(header, errorMessage) || !ValidateHeader(header, resolvedPath, errorMessage))
	{
		Diagnostics::Fail(
		    g_cookedTextureAssetLoaderLogger,
		    __FILE__,
		    __LINE__,
		    std::format("CookedTextureAssetLoader: {}", errorMessage));
		return {};
	}

	TextureFormatIntent formatIntent = TextureFormatIntent::Unknown;
	if (!TryResolveFormatIntent(header.formatIntent, formatIntent))
	{
		Diagnostics::Fail(
		    g_cookedTextureAssetLoaderLogger,
		    __FILE__,
		    __LINE__,
		    std::format(
		        "CookedTextureAssetLoader: '{}' stores an invalid texture format intent {}",
		        resolvedPath.string(),
		        header.formatIntent));
		return {};
	}

	std::vector<CookedTextureMipHeader> mipHeaders;
	if (!ReadMipHeaders(reader, header.mipCount * header.GetArraySize(), resolvedPath, mipHeaders, errorMessage))
	{
		Diagnostics::Fail(
		    g_cookedTextureAssetLoaderLogger,
		    __FILE__,
		    __LINE__,
		    std::format("CookedTextureAssetLoader: {}", errorMessage));
		return {};
	}

	TextureLoadResult loadResult;
	loadResult.width = header.width;
	loadResult.height = header.height;
	loadResult.arraySize = header.GetArraySize();
	loadResult.dimension = header.GetDimension();
	loadResult.dxgiFormat = static_cast<DXGI_FORMAT>(header.dxgiFormat);
	loadResult.formatIntent = formatIntent;
	if (!ReadMipPayloads(reader, mipHeaders, resolvedPath, loadResult, errorMessage))
	{
		Diagnostics::Fail(
		    g_cookedTextureAssetLoaderLogger,
		    __FILE__,
		    __LINE__,
		    std::format("CookedTextureAssetLoader: {}", errorMessage));
		return {};
	}

	if (reader.GetRemainingByteCount() != 0)
	{
		Diagnostics::Fail(
		    g_cookedTextureAssetLoaderLogger,
		    __FILE__,
		    __LINE__,
		    std::format(
		        "CookedTextureAssetLoader: '{}' contains {} unexpected trailing byte(s)",
		        resolvedPath.string(),
		        reader.GetRemainingByteCount()));
		return {};
	}

	return loadResult;
}

bool CookedTextureAssetLoader::ValidateHeader(
    const CookedTextureAssetHeader& header,
    const std::filesystem::path& resolvedPath,
    std::string& outErrorMessage)
{
	if (!header.MatchesExpectedLayout())
	{
		outErrorMessage = "Invalid cooked texture asset header for '" + resolvedPath.string() + "'";
		return false;
	}

	if (header.width == 0 || header.height == 0 || header.mipCount == 0 || header.dxgiFormat == DXGI_FORMAT_UNKNOWN)
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

bool CookedTextureAssetLoader::ValidateMipHeader(
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

bool CookedTextureAssetLoader::ReadMipHeaders(
	Files::BinarySpanReader& reader,
    std::uint32_t mipCount,
    const std::filesystem::path& resolvedPath,
    std::vector<CookedTextureMipHeader>& outMipHeaders,
    std::string& outErrorMessage)
{
	outMipHeaders.clear();
	if (!reader.ReadArray(mipCount, outMipHeaders, outErrorMessage))
	{
		return false;
	}

	for (std::uint32_t mipIndex = 0; mipIndex < mipCount; ++mipIndex)
	{
		if (!ValidateMipHeader(outMipHeaders[mipIndex], mipIndex, resolvedPath, outErrorMessage))
		{
			return false;
		}
	}

	return true;
}

bool CookedTextureAssetLoader::ReadMipPayloads(
	Files::BinarySpanReader& reader,
    const std::vector<CookedTextureMipHeader>& mipHeaders,
    const std::filesystem::path& resolvedPath,
    TextureLoadResult& outLoadResult,
    std::string& outErrorMessage)
{
	outLoadResult.arraySlices.clear();
	outLoadResult.arraySlices.resize(outLoadResult.arraySize);

	if (mipHeaders.empty() || (mipHeaders.size() % outLoadResult.arraySize) != 0)
	{
		outErrorMessage = "Cooked texture asset '" + resolvedPath.string() + "' has an invalid subresource layout.";
		return false;
	}

	const std::uint32_t mipCount = static_cast<std::uint32_t>(mipHeaders.size() / outLoadResult.arraySize);

	for (std::uint32_t subresourceIndex = 0; subresourceIndex < static_cast<std::uint32_t>(mipHeaders.size()); ++subresourceIndex)
	{
		const CookedTextureMipHeader& mipHeader = mipHeaders[subresourceIndex];
		if (!ValidateMipHeader(mipHeader, subresourceIndex, resolvedPath, outErrorMessage))
		{
			return false;
		}

		TextureMipLevelData mipLevel;
		mipLevel.width = mipHeader.width;
		mipLevel.height = mipHeader.height;
		mipLevel.rowPitch = mipHeader.rowPitch;
		mipLevel.slicePitch = mipHeader.slicePitch;

		std::span<const std::uint8_t> mipPayload;
		if (!reader.ReadBytes(mipHeader.dataSize, mipPayload, outErrorMessage))
		{
			outErrorMessage = "Cooked texture asset '" + resolvedPath.string() + "' ended before mip payload data could be read.";
			return false;
		}
		mipLevel.data.assign(mipPayload.begin(), mipPayload.end());

		const std::uint32_t arraySliceIndex = subresourceIndex / mipCount;
		outLoadResult.arraySlices[arraySliceIndex].mipLevels.push_back(std::move(mipLevel));
	}

	return true;
}

bool CookedTextureAssetLoader::TryResolveFormatIntent(std::uint32_t storedValue, TextureFormatIntent& outFormatIntent) noexcept
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