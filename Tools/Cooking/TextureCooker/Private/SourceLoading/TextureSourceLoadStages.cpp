#include "PCH.h"

#include "SourceLoading/TextureSourceLoadStages.h"

#include "Core/Public/FileSystemUtils.h"
#include "Core/Public/Diagnostics/Error.h"
#include "Core/Public/Files/FileUtils.h"

#include <cstring>
#include <format>

TextureSourceFile TextureSourceLoadStages::ReadSourceFile(const std::filesystem::path& sourcePath)
{
	const auto resolvedPathResult = Filesystem::ResolveAssetPathNormalized(sourcePath, AssetType::Texture);
	if (!resolvedPathResult)
	{
		throw Diagnostics::Error(std::format("Failed to resolve source texture '{}'.", sourcePath.string()));
	}

	TextureSourceFile sourceFile;
	sourceFile.ResolvedPath = *resolvedPathResult;
	std::string fileError;
	if (!Files::TryReadAllBytes(sourceFile.ResolvedPath, sourceFile.Bytes, fileError))
	{
		throw Diagnostics::Error(std::format(
		    "Failed to read source texture '{}': {}",
		    sourceFile.ResolvedPath.string(),
		    fileError));
	}

	return sourceFile;
}

TextureLoadResult TextureSourceLoadStages::BuildByteTextureLoadResult(
	int width,
	int height,
	const std::uint8_t* pixelBytes,
	std::size_t pixelByteCount)
{
	if (pixelBytes == nullptr || width <= 0 || height <= 0)
	{
		throw Diagnostics::Error("Decoded raster texture has invalid dimensions or no pixel data.");
	}

	TextureMipLevelData baseMip;
	baseMip.width = static_cast<std::uint32_t>(width);
	baseMip.height = static_cast<std::uint32_t>(height);
	baseMip.rowPitch = static_cast<std::uint32_t>(4u * baseMip.width);
	baseMip.slicePitch = baseMip.rowPitch * baseMip.height;
	if (pixelByteCount < baseMip.slicePitch)
	{
		throw Diagnostics::Error("Decoded raster texture payload is smaller than its RGBA surface.");
	}

	baseMip.data.resize(baseMip.slicePitch);
	std::memcpy(baseMip.data.data(), pixelBytes, baseMip.data.size());

	TextureLoadResult loadResult;
	loadResult.width = baseMip.width;
	loadResult.height = baseMip.height;
	loadResult.arraySize = 1;
	loadResult.dimension = TextureResourceDimension::Texture2D;
	loadResult.dxgiFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	loadResult.formatIntent = TextureFormatIntent::Unknown;
	loadResult.arraySlices.resize(1);
	loadResult.arraySlices.front().push_back(std::move(baseMip));

	return loadResult;
}

TextureLoadResult TextureSourceLoadStages::BuildFloatTextureLoadResult(
	int width,
	int height,
	const float* pixelBytes,
	std::size_t pixelFloatCount)
{
	if (pixelBytes == nullptr || width <= 0 || height <= 0)
	{
		throw Diagnostics::Error("Decoded HDR texture has invalid dimensions or no pixel data.");
	}

	TextureMipLevelData baseMip;
	baseMip.width = static_cast<std::uint32_t>(width);
	baseMip.height = static_cast<std::uint32_t>(height);
	baseMip.rowPitch = static_cast<std::uint32_t>(sizeof(float) * 4u * baseMip.width);
	baseMip.slicePitch = baseMip.rowPitch * baseMip.height;
	const std::size_t surfaceFloatCount = static_cast<std::size_t>(baseMip.width) * static_cast<std::size_t>(baseMip.height) * 4u;
	if (pixelFloatCount < surfaceFloatCount)
	{
		throw Diagnostics::Error("Decoded HDR texture payload is smaller than its RGBA float surface.");
	}

	baseMip.data.resize(baseMip.slicePitch);
	std::memcpy(baseMip.data.data(), pixelBytes, baseMip.data.size());

	TextureLoadResult loadResult;
	loadResult.width = baseMip.width;
	loadResult.height = baseMip.height;
	loadResult.arraySize = 1;
	loadResult.dimension = TextureResourceDimension::Texture2D;
	loadResult.dxgiFormat = DXGI_FORMAT_R32G32B32A32_FLOAT;
	loadResult.formatIntent = TextureFormatIntent::DataLinear;
	loadResult.arraySlices.resize(1);
	loadResult.arraySlices.front().push_back(std::move(baseMip));

	return loadResult;
}
