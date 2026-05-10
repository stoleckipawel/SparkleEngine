#pragma once

#include "D3D12/Textures/TextureLoadResult.h"

#include <filesystem>
#include <string>

class TextureLoader final
{
  public:
	static TextureLoadResult Load(const std::filesystem::path& fileName);

	TextureLoader() = delete;
	~TextureLoader() = delete;
};