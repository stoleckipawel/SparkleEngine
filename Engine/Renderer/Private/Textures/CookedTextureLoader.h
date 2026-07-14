#pragma once

#include "RHI/Public/Resources/RhiTextureUpload.h"

#include <filesystem>

struct LoadedTextureData final
{
	RhiTextureUploadDesc Upload;
	TextureFormatIntent FormatIntent = TextureFormatIntent::Unknown;

	bool IsValid() const noexcept { return Upload.IsValid(); }
};

class CookedTextureLoader final
{
  public:
	static LoadedTextureData Load(const std::filesystem::path& texturePath);

	CookedTextureLoader() = delete;
	~CookedTextureLoader() = delete;
};
