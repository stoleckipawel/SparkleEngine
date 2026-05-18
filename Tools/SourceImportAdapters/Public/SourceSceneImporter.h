#pragma once

#include "SourceImportResult.h"

#include <filesystem>

class SourceSceneImporter final
{
  public:
	static bool SupportsSourceScenePath(const std::filesystem::path& filePath);
	static SourceImportResult Import(const std::filesystem::path& filePath);

	SourceSceneImporter() = delete;
	~SourceSceneImporter() = delete;
};


