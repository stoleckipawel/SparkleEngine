#pragma once

#include "GameFramework/Public/Assets/SceneImportResult.h"
#include "GameFramework/Public/GameFrameworkAPI.h"

#include <filesystem>

class SPARKLE_ENGINE_API SceneImporter final
{
  public:
	static SceneImportResult Load(const std::filesystem::path& filePath);

	SceneImporter() = delete;
	~SceneImporter() = delete;
};