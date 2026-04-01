#pragma once

#include "GameFramework/Public/Assets/SceneImportResult.h"

#include <filesystem>

class FbxImporter final
{
  public:
	static SceneImportResult Load(const std::filesystem::path& filePath);

	FbxImporter() = delete;
	~FbxImporter() = delete;
};