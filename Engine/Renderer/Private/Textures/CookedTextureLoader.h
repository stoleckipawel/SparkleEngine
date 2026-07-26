#pragma once

#include "RHI/Public/Resources/RhiTextureUpload.h"

#include <filesystem>
#include <string>
#include <vector>

struct LoadedTextureData final
{
	RhiTextureUploadDesc Upload;
	TextureFormatIntent FormatIntent = TextureFormatIntent::Unknown;

	bool IsValid() const noexcept { return Upload.IsValid(); }
};

struct CookedTextureFilePayload final
{
	std::filesystem::path ResolvedPath;
	std::vector<std::uint8_t> Bytes;
};

class CookedTextureLoader final
{
  public:
	static bool TryRead(
	    const std::filesystem::path& texturePath,
	    CookedTextureFilePayload& outPayload,
	    std::string& outErrorMessage);
	static bool TryDecode(
	    const CookedTextureFilePayload& payload,
	    LoadedTextureData& outTexture,
	    std::string& outErrorMessage);

	CookedTextureLoader() = delete;
	~CookedTextureLoader() = delete;
};
