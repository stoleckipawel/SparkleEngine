#include "PCH.h"

#include "Cooking/TextureSourceLoaderUtils.h"

#include "Core/Public/FileSystemUtils.h"
#include "Core/Public/Files/FileUtils.h"

#include <cstring>
#include <format>

bool TextureSourceLoaderUtils::TryReadSourceBytes(
	const std::filesystem::path& sourcePath,
	std::filesystem::path& outResolvedPath,
	std::vector<std::uint8_t>& outFileBytes,
	std::string& outErrorMessage)
{
	const auto resolvedPathResult = Filesystem::ResolveAssetPathNormalized(sourcePath, AssetType::Texture);
	if (!resolvedPathResult)
	{
		outErrorMessage = std::format("Failed to resolve source texture '{}'", sourcePath.string());
		return false;
	}

	outResolvedPath = *resolvedPathResult;
	if (!Files::TryReadAllBytes(outResolvedPath, outFileBytes, outErrorMessage))
	{
		outErrorMessage = std::format("Failed to read source texture '{}': {}", outResolvedPath.string(), outErrorMessage);
		return false;
	}

	return true;
}

TextureLoadResult TextureSourceLoaderUtils::BuildByteTextureLoadResult(
	int width,
	int height,
	const std::uint8_t* pixelBytes,
	std::size_t pixelByteCount,
	std::string& outErrorMessage)
{
	if (pixelBytes == nullptr || width <= 0 || height <= 0)
	{
		outErrorMessage = "Decoded raster texture is invalid.";
		return {};
	}

	TextureMipLevelData baseMip;
	baseMip.width = static_cast<std::uint32_t>(width);
	baseMip.height = static_cast<std::uint32_t>(height);
	baseMip.rowPitch = static_cast<std::uint32_t>(4u * baseMip.width);
	baseMip.slicePitch = baseMip.rowPitch * baseMip.height;
	if (pixelByteCount < baseMip.slicePitch)
	{
		outErrorMessage = "Decoded raster texture payload is smaller than the expected RGBA surface size.";
		return {};
	}

	baseMip.data.resize(baseMip.slicePitch);
	std::memcpy(baseMip.data.data(), pixelBytes, baseMip.data.size());

	TextureLoadResult loadResult;
	loadResult.width = baseMip.width;
	loadResult.height = baseMip.height;
	loadResult.dxgiFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	loadResult.formatIntent = TextureFormatIntent::Unknown;
	loadResult.mipLevels.push_back(std::move(baseMip));

	outErrorMessage.clear();
	return loadResult;
}

TextureLoadResult TextureSourceLoaderUtils::BuildFloatTextureLoadResult(
	int width,
	int height,
	const float* pixelBytes,
	std::size_t pixelFloatCount,
	std::string& outErrorMessage)
{
	if (pixelBytes == nullptr || width <= 0 || height <= 0)
	{
		outErrorMessage = "Decoded HDR texture is invalid.";
		return {};
	}

	TextureMipLevelData baseMip;
	baseMip.width = static_cast<std::uint32_t>(width);
	baseMip.height = static_cast<std::uint32_t>(height);
	baseMip.rowPitch = static_cast<std::uint32_t>(sizeof(float) * 4u * baseMip.width);
	baseMip.slicePitch = baseMip.rowPitch * baseMip.height;
	const std::size_t requiredFloatCount = static_cast<std::size_t>(baseMip.width) * static_cast<std::size_t>(baseMip.height) * 4u;
	if (pixelFloatCount < requiredFloatCount)
	{
		outErrorMessage = "Decoded HDR texture payload is smaller than the expected RGBA float surface size.";
		return {};
	}

	baseMip.data.resize(baseMip.slicePitch);
	std::memcpy(baseMip.data.data(), pixelBytes, baseMip.data.size());

	TextureLoadResult loadResult;
	loadResult.width = baseMip.width;
	loadResult.height = baseMip.height;
	loadResult.dxgiFormat = DXGI_FORMAT_R32G32B32A32_FLOAT;
	loadResult.formatIntent = TextureFormatIntent::DataLinear;
	loadResult.mipLevels.push_back(std::move(baseMip));

	outErrorMessage.clear();
	return loadResult;
}