#include "PCH.h"

#include "SourceLoading/TextureSourceLoader.h"

#include "SourceLoading/TextureSourceLoadStages.h"

#include "Core/Public/Diagnostics/Error.h"

#define TINYEXR_IMPLEMENTATION
#include <tinyexr.h>

#include <cstdlib>
#include <format>
#include <memory>

TextureLoadResult LoadExrTextureSource(const std::filesystem::path& sourcePath)
{
	const TextureSourceFile sourceFile = ReadTextureSourceFile(sourcePath);

	float* decodedPixels = nullptr;
	int width = 0;
	int height = 0;
	const char* errorMessage = nullptr;
	const int result = LoadEXRFromMemory(&decodedPixels, &width, &height, sourceFile.Bytes.data(), sourceFile.Bytes.size(), &errorMessage);
	if (result != TINYEXR_SUCCESS || decodedPixels == nullptr)
	{
		std::string diagnostic = std::format("Failed to decode EXR texture '{}'", sourceFile.ResolvedPath.string());
		if (errorMessage != nullptr)
		{
			diagnostic += ": ";
			diagnostic += errorMessage;
			FreeEXRErrorMessage(errorMessage);
		}
		throw Diagnostics::Error(diagnostic);
	}

	std::unique_ptr<float, decltype(&std::free)> pixels(decodedPixels, &std::free);
	return BuildFloatTextureLoadResult(width, height, pixels.get(), static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4u);
}
