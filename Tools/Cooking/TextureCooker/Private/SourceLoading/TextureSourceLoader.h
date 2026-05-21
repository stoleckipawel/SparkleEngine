#pragma once

#include "SourceLoading/TextureSourceLoaderBackend.h"

#include <filesystem>
#include <string>

class TextureSourceLoader final
{
  public:
	static TextureLoadResult Load(const std::filesystem::path& sourcePath, std::string& outErrorMessage);

  private:
	static TextureSourceFormat ResolveFormat(const std::filesystem::path& sourcePath) noexcept;
};