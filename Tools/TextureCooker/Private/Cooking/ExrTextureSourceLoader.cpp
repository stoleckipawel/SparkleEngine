#include "PCH.h"

#include "Cooking/ExrTextureSourceLoader.h"

#include "Cooking/TextureSourceLoaderUtils.h"

#define TINYEXR_IMPLEMENTATION
#include <tinyexr.h>

bool ExrTextureSourceLoader::SupportsFormat(TextureSourceFormat format) const noexcept
{
	return format == TextureSourceFormat::Exr;
}

TextureLoadResult ExrTextureSourceLoader::Load(const std::filesystem::path& sourcePath, std::string& outErrorMessage) const
{
	std::filesystem::path resolvedPath;
	std::vector<std::uint8_t> fileBytes;
	if (!TextureSourceLoaderUtils::TryReadSourceBytes(sourcePath, resolvedPath, fileBytes, outErrorMessage))
	{
		return {};
	}

	float* pixels = nullptr;
	int width = 0;
	int height = 0;
	const char* errorMessage = nullptr;
	const int result = LoadEXRFromMemory(&pixels, &width, &height, fileBytes.data(), fileBytes.size(), &errorMessage);
	if (result != TINYEXR_SUCCESS || pixels == nullptr)
	{
		outErrorMessage = std::format("Failed to decode EXR texture '{}'", resolvedPath.string());
		if (errorMessage != nullptr)
		{
			outErrorMessage += ": ";
			outErrorMessage += errorMessage;
			FreeEXRErrorMessage(errorMessage);
		}
		return {};
	}

	TextureLoadResult loadResult = TextureSourceLoaderUtils::BuildFloatTextureLoadResult(
	    width,
	    height,
	    pixels,
	    static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4u,
	    outErrorMessage);
	free(pixels);
	return loadResult;
}