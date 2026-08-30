#pragma once

#include "SourceLoading/TextureSourceLoaderBackend.h"

#include <filesystem>

class TextureSourceLoader final
{
public:
	static TextureLoadResult Load(const std::filesystem::path& sourcePath);

private:
	static TextureSourceFormat ResolveFormat(const std::filesystem::path& sourcePath) noexcept;
};
