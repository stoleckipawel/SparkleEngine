#include "PCH.h"

#include "Cooking/HdrTextureSourceLoader.h"

#include "Cooking/TextureSourceLoaderUtils.h"

#include <stb_image.h>

bool HdrTextureSourceLoader::SupportsFormat(TextureSourceFormat format) const noexcept
{
	return format == TextureSourceFormat::RadianceHdr;
}

TextureLoadResult HdrTextureSourceLoader::Load(const std::filesystem::path& sourcePath, std::string& outErrorMessage) const
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
	float* pixels = stbi_loadf_from_memory(
	    fileBytes.data(),
	    static_cast<int>(fileBytes.size()),
	    &width,
	    &height,
	    &sourceChannels,
	    STBI_rgb_alpha);
	if (pixels == nullptr)
	{
		outErrorMessage = std::format(
		    "Failed to decode HDR texture '{}': {}",
		    resolvedPath.string(),
		    stbi_failure_reason() != nullptr ? stbi_failure_reason() : "unknown stb_image error");
		return {};
	}

	TextureLoadResult loadResult = TextureSourceLoaderUtils::BuildFloatTextureLoadResult(
	    width,
	    height,
	    pixels,
	    static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4u,
	    outErrorMessage);
	stbi_image_free(pixels);
	return loadResult;
}