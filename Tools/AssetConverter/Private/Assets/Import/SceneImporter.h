#pragma once

#include "Assets/Import/SceneImportResult.h"

#include <filesystem>

class SceneImporter final
{
  public:
	static SceneImportResult Import(const std::filesystem::path& filePath);

	SceneImporter() = delete;
	~SceneImporter() = delete;
};