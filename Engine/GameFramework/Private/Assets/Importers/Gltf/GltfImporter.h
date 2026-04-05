#pragma once

#include "Assets/Import/SceneImportResult.h"
#include "Assets/Importers/AssetImporter.h"

#include <filesystem>
#include <string_view>

class GltfImporter final : public AssetImporter
{
  public:
	bool SupportsExtension(std::wstring_view extension) const noexcept override;
	SceneImportResult Import(const std::filesystem::path& filePath) const override;
};