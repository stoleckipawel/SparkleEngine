#pragma once

#include "Assets/Import/SceneImportResult.h"
#include "Editor/Public/EditorAPI.h"

#include <filesystem>

class SPARKLE_EDITOR_API SceneImporter final
{
  public:
	static SceneImportResult Import(const std::filesystem::path& filePath);

	SceneImporter() = delete;
	~SceneImporter() = delete;
};