#pragma once

#include "Pipeline/TextureLoadResult.h"

#include <cstdint>
#include <filesystem>

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
	TextureSourceLoaderBackend(const TextureSourceLoaderBackend&) = delete;
	TextureSourceLoaderBackend& operator=(const TextureSourceLoaderBackend&) = delete;
	TextureSourceLoaderBackend(TextureSourceLoaderBackend&&) = delete;
	TextureSourceLoaderBackend& operator=(TextureSourceLoaderBackend&&) = delete;

	virtual bool SupportsFormat(TextureSourceFormat format) const noexcept = 0;
	virtual TextureLoadResult Load(const std::filesystem::path& sourcePath) const = 0;

protected:
	TextureSourceLoaderBackend() = default;
};
