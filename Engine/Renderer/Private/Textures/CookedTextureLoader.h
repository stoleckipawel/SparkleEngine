#pragma once

#include "RHI/Public/Resources/RhiTextureUpload.h"

#include <filesystem>

class CookedTextureLoader final
{
  public:
	static RhiTextureUploadDesc Load(const std::filesystem::path& texturePath);

	CookedTextureLoader() = delete;
	~CookedTextureLoader() = delete;
};