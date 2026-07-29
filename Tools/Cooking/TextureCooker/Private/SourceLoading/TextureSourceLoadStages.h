#pragma once

#include "Pipeline/TextureLoadResult.h"

#include <filesystem>
#include <vector>

struct TextureSourceFile final
{
	std::filesystem::path ResolvedPath;
	std::vector<std::uint8_t> Bytes;
};

class TextureSourceLoadStages final
{
  public:
	static TextureSourceFile ReadSourceFile(const std::filesystem::path& sourcePath);

	static TextureLoadResult BuildByteTextureLoadResult(
	    int width,
	    int height,
	    const std::uint8_t* pixelBytes,
	    std::size_t pixelByteCount);

	static TextureLoadResult BuildFloatTextureLoadResult(
	    int width,
	    int height,
	    const float* pixelBytes,
	    std::size_t pixelFloatCount);
};
