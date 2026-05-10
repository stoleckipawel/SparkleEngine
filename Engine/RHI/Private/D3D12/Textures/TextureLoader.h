#pragma once

#include "D3D12/Textures/TextureLoadResult.h"
#include "RHIAPI.h"

#include <filesystem>
#include <string>

class SPARKLE_RHI_API TextureLoader final
{
  public:
	static TextureLoadResult Load(const std::filesystem::path& fileName);

	TextureLoader() = delete;
	~TextureLoader() = delete;
};