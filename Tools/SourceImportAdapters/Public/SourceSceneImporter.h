#pragma once

#include "SourceImportResult.h"

#include <filesystem>

class SourceSceneImporter final
{
  public:
	static SourceImportResult Import(const std::filesystem::path& filePath);

	SourceSceneImporter() = delete;
	~SourceSceneImporter() = delete;
};


