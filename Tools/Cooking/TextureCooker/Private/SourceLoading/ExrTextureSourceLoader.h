#pragma once

#include "SourceLoading/TextureSourceLoaderBackend.h"

class ExrTextureSourceLoader final : public TextureSourceLoaderBackend
{
  public:
	bool SupportsFormat(TextureSourceFormat format) const noexcept override;
	TextureLoadResult Load(const std::filesystem::path& sourcePath, std::string& outErrorMessage) const override;
};