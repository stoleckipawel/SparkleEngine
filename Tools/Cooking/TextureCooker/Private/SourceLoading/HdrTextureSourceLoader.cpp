#include "PCH.h"

#include "SourceLoading/HdrTextureSourceLoader.h"

#include "SourceLoading/TextureSourceLoadStages.h"

#include "Core/Public/Diagnostics/Error.h"

#include <stb_image.h>

#include <format>
#include <memory>

bool HdrTextureSourceLoader::SupportsFormat(TextureSourceFormat format) const noexcept
{
	return format == TextureSourceFormat::RadianceHdr;
}

TextureLoadResult HdrTextureSourceLoader::Load(const std::filesystem::path& sourcePath) const
{
	const TextureSourceFile sourceFile = TextureSourceLoadStages::ReadSourceFile(sourcePath);

	int width = 0;
	int height = 0;
	int sourceChannels = 0;
	std::unique_ptr<float, decltype(&stbi_image_free)> pixels(
	    stbi_loadf_from_memory(
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
		        "Failed to decode HDR texture '{}': {}",
		        sourceFile.ResolvedPath.string(),
		        stbi_failure_reason() != nullptr ? stbi_failure_reason() : "unknown stb_image error"));
	}

	return TextureSourceLoadStages::BuildFloatTextureLoadResult(
	    width,
	    height,
	    pixels.get(),
	    static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4u);
}
