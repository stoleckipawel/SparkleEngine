#pragma once

#include "RHI/Public/Resources/RhiTextureUpload.h"

#include <filesystem>
#include <vector>

struct LoadedTextureData final
{
	RhiTextureUploadDesc Upload;
	TextureFormatIntent FormatIntent = TextureFormatIntent::Unknown;
};

struct CookedTextureFilePayload final
{
	std::filesystem::path ResolvedPath;
	std::vector<std::uint8_t> Bytes;
};

class CookedTextureLoader final
{
  public:
	static CookedTextureFilePayload Read(const std::filesystem::path& texturePath);
	static LoadedTextureData Decode(const CookedTextureFilePayload& payload);

	CookedTextureLoader() = delete;
	~CookedTextureLoader() = delete;
};
