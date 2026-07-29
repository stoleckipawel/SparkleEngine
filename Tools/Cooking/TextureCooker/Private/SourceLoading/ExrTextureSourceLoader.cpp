#include "PCH.h"

#include "SourceLoading/ExrTextureSourceLoader.h"

#include "SourceLoading/TextureSourceLoadStages.h"

#include "Core/Public/Diagnostics/Error.h"

#define TINYEXR_IMPLEMENTATION
#include <tinyexr.h>

#include <cstdlib>
#include <format>
#include <memory>

bool ExrTextureSourceLoader::SupportsFormat(TextureSourceFormat format) const noexcept
{
	return format == TextureSourceFormat::Exr;
}

TextureLoadResult ExrTextureSourceLoader::Load(const std::filesystem::path& sourcePath) const
{
	const TextureSourceFile sourceFile = TextureSourceLoadStages::ReadSourceFile(sourcePath);

	float* decodedPixels = nullptr;
	int width = 0;
	int height = 0;
	const char* errorMessage = nullptr;
	const int result = LoadEXRFromMemory(
	    &decodedPixels,
	    &width,
	    &height,
	    sourceFile.Bytes.data(),
	    sourceFile.Bytes.size(),
	    &errorMessage);
	if (result != TINYEXR_SUCCESS || decodedPixels == nullptr)
	{
		std::string diagnostic = std::format("Failed to decode EXR texture '{}'", sourceFile.ResolvedPath.string());
		if (errorMessage != nullptr)
		{
			diagnostic += ": ";
			diagnostic += errorMessage;
			FreeEXRErrorMessage(errorMessage);
		}
		throw Diagnostics::Error(std::move(diagnostic));
	}

	std::unique_ptr<float, decltype(&std::free)> pixels(decodedPixels, &std::free);
	return TextureSourceLoadStages::BuildFloatTextureLoadResult(
	    width,
	    height,
	    pixels.get(),
	    static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4u);
}
