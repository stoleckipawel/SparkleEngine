#pragma once

#include "D3D12/Textures/TextureLoadResult.h"

#include <filesystem>
#include <string_view>

class TextureLoaderBackend
{
  public:
	virtual ~TextureLoaderBackend() = default;

	virtual bool SupportsExtension(std::wstring_view extension) const noexcept = 0;
	virtual TextureLoadResult Load(const std::filesystem::path& fileName) const = 0;
};