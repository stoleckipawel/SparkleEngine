#include "PCH.h"

#include "SourceLoading/TextureSourceLoadStages.h"

#include "Core/Public/FileSystemUtils.h"
#include "Core/Public/Diagnostics/Error.h"
#include "Core/Public/Files/FileUtils.h"

#include <cstring>
#include <format>
#include <limits>
#include <string_view>

TextureSourceFile ReadTextureSourceFile(const std::filesystem::path& sourcePath)
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
		throw Diagnostics::Error(std::format("Failed to read source texture '{}': {}", sourceFile.ResolvedPath.string(), fileError));
	}

	return sourceFile;
}

template <typename T> static TextureLoadResult BuildDecodedTexture(
    int width,
    int height,
    const T* pixels,
    std::size_t elementCount,
    DXGI_FORMAT format,
    TextureFormatIntent intent,
    std::string_view sourceKind)
{
	if (pixels == nullptr || width <= 0 || height <= 0)
	{
		throw Diagnostics::Error(std::format("Decoded {} texture has invalid dimensions or no pixel data.", sourceKind));
	}

	const std::uint64_t rowPitch = static_cast<std::uint64_t>(width) * 4u * sizeof(T);
	if (rowPitch > (std::numeric_limits<std::uint32_t>::max)())
	{
		throw Diagnostics::Error("Decoded texture row pitch exceeds the supported surface size.");
	}
	const std::uint64_t slicePitch = rowPitch * static_cast<std::uint64_t>(height);
	if (slicePitch > (std::numeric_limits<std::uint32_t>::max)())
	{
		throw Diagnostics::Error("Decoded texture payload exceeds the supported surface size.");
	}
	if (elementCount < slicePitch / sizeof(T))
	{
		throw Diagnostics::Error(
		    std::format("Decoded {} texture payload is smaller than its RGBA{} surface.", sourceKind, sizeof(T) == 1 ? "" : " float"));
	}

	TextureMipLevelData baseMip;
	baseMip.width = static_cast<std::uint32_t>(width);
	baseMip.height = static_cast<std::uint32_t>(height);
	baseMip.rowPitch = static_cast<std::uint32_t>(rowPitch);
	baseMip.slicePitch = static_cast<std::uint32_t>(slicePitch);
	baseMip.data.resize(baseMip.slicePitch);
	std::memcpy(baseMip.data.data(), pixels, baseMip.data.size());

	TextureLoadResult loadResult;
	loadResult.width = baseMip.width;
	loadResult.height = baseMip.height;
	loadResult.arraySize = 1;
	loadResult.dimension = TextureResourceDimension::Texture2D;
	loadResult.dxgiFormat = format;
	loadResult.formatIntent = intent;
	loadResult.arraySlices.resize(1);
	loadResult.arraySlices.front().push_back(std::move(baseMip));
	return loadResult;
}

TextureLoadResult BuildByteTextureLoadResult(int width, int height, const std::uint8_t* pixels, std::size_t pixelByteCount)
{
	return BuildDecodedTexture(width, height, pixels, pixelByteCount, DXGI_FORMAT_R8G8B8A8_UNORM, TextureFormatIntent::Unknown, "raster");
}

TextureLoadResult BuildFloatTextureLoadResult(int width, int height, const float* pixels, std::size_t pixelFloatCount)
{
	return BuildDecodedTexture(
	    width,
	    height,
	    pixels,
	    pixelFloatCount,
	    DXGI_FORMAT_R32G32B32A32_FLOAT,
	    TextureFormatIntent::DataLinear,
	    "HDR");
}
