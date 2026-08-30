#pragma once

#include "SourceImportOutput.h"

#include <filesystem>

class SourceSceneImporter final
{
public:
	static bool SupportsSourceScenePath(const std::filesystem::path& filePath);
	static SourceImportOutput Import(const std::filesystem::path& filePath);

	SourceSceneImporter() = delete;
	~SourceSceneImporter() = delete;
};
