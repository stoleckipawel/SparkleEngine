#pragma once

#include "D3D12/Textures/TextureLoadResult.h"

#include <cstdint>
#include <filesystem>
#include <string>

enum class TextureSourceFormat : std::uint8_t
{
	Unknown,
	StandardRaster,
	RadianceHdr,
	Dds,
	Exr
};

class TextureSourceLoaderBackend
{
  public:
	virtual ~TextureSourceLoaderBackend() = default;

	virtual bool SupportsFormat(TextureSourceFormat format) const noexcept = 0;
	virtual TextureLoadResult Load(const std::filesystem::path& sourcePath, std::string& outErrorMessage) const = 0;
};