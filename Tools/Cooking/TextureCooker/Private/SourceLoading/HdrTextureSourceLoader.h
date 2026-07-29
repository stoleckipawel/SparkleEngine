#pragma once

#include "SourceLoading/TextureSourceLoaderBackend.h"

class HdrTextureSourceLoader final : public TextureSourceLoaderBackend
{
  public:
	bool SupportsFormat(TextureSourceFormat format) const noexcept override;
	TextureLoadResult Load(const std::filesystem::path& sourcePath) const override;
};
