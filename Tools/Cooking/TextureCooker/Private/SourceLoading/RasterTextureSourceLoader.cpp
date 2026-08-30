#include "PCH.h"

#include "SourceLoading/RasterTextureSourceLoader.h"

#include "SourceLoading/TextureSourceLoadStages.h"

#include "Core/Public/Diagnostics/Error.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <format>
#include <memory>

bool RasterTextureSourceLoader::SupportsFormat(TextureSourceFormat format) const noexcept
{
	return format == TextureSourceFormat::StandardRaster;
}

TextureLoadResult RasterTextureSourceLoader::Load(const std::filesystem::path& sourcePath) const
{
	const TextureSourceFile sourceFile = TextureSourceLoadStages::ReadSourceFile(sourcePath);

	int width = 0;
	int height = 0;
	int sourceChannels = 0;
	std::unique_ptr<stbi_uc, decltype(&stbi_image_free)> pixels(
	    stbi_load_from_memory(
	        sourceFile.Bytes.data(),
	        static_cast<int>(sourceFile.Bytes.size()),
	        &width,
	        &height,
	        &sourceChannels,
	        STBI_rgb_alpha),
	    &stbi_image_free);
	if (!pixels)
	{
		throw Diagnostics::Error(
		    std::format(
		        "Failed to decode raster texture '{}': {}",
		        sourceFile.ResolvedPath.string(),
		        stbi_failure_reason() != nullptr ? stbi_failure_reason() : "unknown stb_image error"));
	}

	return TextureSourceLoadStages::BuildByteTextureLoadResult(
	    width,
	    height,
	    pixels.get(),
	    static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4u);
}
