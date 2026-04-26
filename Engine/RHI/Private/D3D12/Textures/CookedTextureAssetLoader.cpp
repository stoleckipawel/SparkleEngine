#include "PCH.h"

#include "D3D12/Textures/CookedTextureAssetLoader.h"

#include "Core/Public/Files/FileUtils.h"
#include "Core/Public/FileSystemUtils.h"

#include <cstring>
#include <format>

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

	std::size_t byteOffset = 0;
	CookedTextureAssetHeader header;
	if (!ReadBytes(fileBytes, byteOffset, &header, sizeof(header), errorMessage) || !ValidateHeader(header, resolvedPath, errorMessage))
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
	if (!ReadMipHeaders(fileBytes, byteOffset, header.mipCount, resolvedPath, mipHeaders, errorMessage))
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
	loadResult.dxgiFormat = static_cast<DXGI_FORMAT>(header.dxgiFormat);
	loadResult.formatIntent = formatIntent;
	if (!ReadMipPayloads(fileBytes, byteOffset, mipHeaders, resolvedPath, loadResult, errorMessage))
	{
		Diagnostics::Fail(
		    g_cookedTextureAssetLoaderLogger,
		    __FILE__,
		    __LINE__,
		    std::format("CookedTextureAssetLoader: {}", errorMessage));
		return {};
	}

	if (byteOffset != fileBytes.size())
	{
		Diagnostics::Fail(
		    g_cookedTextureAssetLoaderLogger,
		    __FILE__,
		    __LINE__,
		    std::format(
		        "CookedTextureAssetLoader: '{}' contains {} unexpected trailing byte(s)",
		        resolvedPath.string(),
		        fileBytes.size() - byteOffset));
		return {};
	}

	return loadResult;
}

bool CookedTextureAssetLoader::ReadBytes(
    const std::vector<std::uint8_t>& fileBytes,
    std::size_t& byteOffset,
    void* destination,
    std::size_t byteCount,
    std::string& outErrorMessage)
{
	if (byteOffset + byteCount > fileBytes.size())
	{
		outErrorMessage = "Cooked texture asset ended before the expected payload could be read.";
		return false;
	}

	std::memcpy(destination, fileBytes.data() + byteOffset, byteCount);
	byteOffset += byteCount;
	return true;
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
    const std::vector<std::uint8_t>& fileBytes,
    std::size_t& byteOffset,
    std::uint32_t mipCount,
    const std::filesystem::path& resolvedPath,
    std::vector<CookedTextureMipHeader>& outMipHeaders,
    std::string& outErrorMessage)
{
	outMipHeaders.clear();
	outMipHeaders.resize(mipCount);

	for (std::uint32_t mipIndex = 0; mipIndex < mipCount; ++mipIndex)
	{
		if (!ReadBytes(fileBytes, byteOffset, &outMipHeaders[mipIndex], sizeof(CookedTextureMipHeader), outErrorMessage))
		{
			return false;
		}

		if (!ValidateMipHeader(outMipHeaders[mipIndex], mipIndex, resolvedPath, outErrorMessage))
		{
			return false;
		}
	}

	return true;
}

bool CookedTextureAssetLoader::ReadMipPayloads(
    const std::vector<std::uint8_t>& fileBytes,
    std::size_t& byteOffset,
    const std::vector<CookedTextureMipHeader>& mipHeaders,
    const std::filesystem::path& resolvedPath,
    TextureLoadResult& outLoadResult,
    std::string& outErrorMessage)
{
	outLoadResult.mipLevels.clear();
	outLoadResult.mipLevels.reserve(mipHeaders.size());

	for (std::uint32_t mipIndex = 0; mipIndex < static_cast<std::uint32_t>(mipHeaders.size()); ++mipIndex)
	{
		const CookedTextureMipHeader& mipHeader = mipHeaders[mipIndex];
		if (!ValidateMipHeader(mipHeader, mipIndex, resolvedPath, outErrorMessage))
		{
			return false;
		}

		TextureMipLevelData mipLevel;
		mipLevel.width = mipHeader.width;
		mipLevel.height = mipHeader.height;
		mipLevel.rowPitch = mipHeader.rowPitch;
		mipLevel.slicePitch = mipHeader.slicePitch;
		mipLevel.data.resize(mipHeader.dataSize);
		if (!ReadBytes(fileBytes, byteOffset, mipLevel.data.data(), mipLevel.data.size(), outErrorMessage))
		{
			outErrorMessage = "Cooked texture asset '" + resolvedPath.string() + "' ended before mip payload data could be read.";
			return false;
		}

		outLoadResult.mipLevels.push_back(std::move(mipLevel));
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