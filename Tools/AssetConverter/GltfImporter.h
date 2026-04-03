#pragma once

#include "GameFramework/Public/Assets/SceneImportResult.h"

#include <filesystem>

class GltfImporter final
{
  public:
	static SceneImportResult Load(const std::filesystem::path& filePath);

	GltfImporter() = delete;
	~GltfImporter() = delete;
};
