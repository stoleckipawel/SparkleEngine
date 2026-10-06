#pragma once

#include "SourceImportOutput.h"

#include <filesystem>

bool SupportsSourceScenePath(const std::filesystem::path& filePath);
SourceImportOutput ImportSourceScene(const std::filesystem::path& filePath);
