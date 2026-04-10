#pragma once

#include "Assets/Import/SceneImportResult.h"

#include <filesystem>
#include <string_view>

class AssetImporter
{
  public:
	virtual ~AssetImporter();

	virtual bool SupportsExtension(std::wstring_view extension) const noexcept = 0;
	virtual SceneImportResult Import(const std::filesystem::path& filePath) const = 0;
};