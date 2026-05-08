#include "PCH.h"

#include "SourceLoading/RasterTextureSourceLoader.h"

#include "SourceLoading/TextureSourceLoaderUtils.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

bool RasterTextureSourceLoader::SupportsFormat(TextureSourceFormat format) const noexcept
{
	return format == TextureSourceFormat::StandardRaster;
}

TextureLoadResult RasterTextureSourceLoader::Load(const std::filesystem::path& sourcePath, std::string& outErrorMessage) const
{
	std::filesystem::path resolvedPath;
	std::vector<std::uint8_t> fileBytes;
	if (!TextureSourceLoaderUtils::TryReadSourceBytes(sourcePath, resolvedPath, fileBytes, outErrorMessage))
	{
		return {};
	}

	int width = 0;
	int height = 0;
	int sourceChannels = 0;
	stbi_uc* pixels = stbi_load_from_memory(
	    fileBytes.data(),
	    static_cast<int>(fileBytes.size()),
	    &width,
	    &height,
	    &sourceChannels,
	    STBI_rgb_alpha);
	if (pixels == nullptr)
	{
		outErrorMessage = std::format(
		    "Failed to decode raster texture '{}': {}",
		    resolvedPath.string(),
		    stbi_failure_reason() != nullptr ? stbi_failure_reason() : "unknown stb_image error");
		return {};
	}

	TextureLoadResult loadResult = TextureSourceLoaderUtils::BuildByteTextureLoadResult(
	    width,
	    height,
	    pixels,
	    static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4u,
	    outErrorMessage);
	stbi_image_free(pixels);
	return loadResult;
}