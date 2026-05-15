#pragma once

#include "RHI/Private/D3D12/Textures/TextureLoadResult.h"

#include <filesystem>
#include <string>

class TextureSourceLoaderUtils final
{
  public:
	static bool TryReadSourceBytes(
	    const std::filesystem::path& sourcePath,
	    std::filesystem::path& outResolvedPath,
	    std::vector<std::uint8_t>& outFileBytes,
	    std::string& outErrorMessage);

	static TextureLoadResult BuildByteTextureLoadResult(
	    int width,
	    int height,
	    const std::uint8_t* pixelBytes,
	    std::size_t pixelByteCount,
	    std::string& outErrorMessage);

	static TextureLoadResult BuildFloatTextureLoadResult(
	    int width,
	    int height,
	    const float* pixelBytes,
	    std::size_t pixelFloatCount,
	    std::string& outErrorMessage);
};