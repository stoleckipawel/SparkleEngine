#pragma once

#include "Pipeline/TextureLoadResult.h"

#include <filesystem>

TextureLoadResult LoadTextureSource(const std::filesystem::path& sourcePath);
TextureLoadResult LoadDdsTextureSource(const std::filesystem::path& sourcePath);
TextureLoadResult LoadExrTextureSource(const std::filesystem::path& sourcePath);
TextureLoadResult LoadHdrTextureSource(const std::filesystem::path& sourcePath);
TextureLoadResult LoadRasterTextureSource(const std::filesystem::path& sourcePath);
