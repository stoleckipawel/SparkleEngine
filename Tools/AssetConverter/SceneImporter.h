#pragma once

#include "GameFramework/Public/Assets/SceneImportResult.h"

#include <filesystem>

class SceneImporter final
{
  public:
	static SceneImportResult Load(const std::filesystem::path& filePath);

	SceneImporter() = delete;
	~SceneImporter() = delete;
};
