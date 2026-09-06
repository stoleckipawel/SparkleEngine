#pragma once

#include "SourceLoading/TextureSourceLoaderBackend.h"

#include <filesystem>

class ExrTextureSourceLoader final : public TextureSourceLoaderBackend
{
public:
	bool SupportsFormat(TextureSourceFormat format) const noexcept override;
	TextureLoadResult Load(const std::filesystem::path& sourcePath) const override;
};
