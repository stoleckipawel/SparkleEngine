#pragma once

#include "SourceImportOutput.h"

#include <filesystem>

SourceImportOutput ImportPlyScene(const std::filesystem::path& filePath);
