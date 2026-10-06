#pragma once

#include "SourceImportOutput.h"

#include <filesystem>

SourceImportOutput ImportFbxScene(const std::filesystem::path& filePath);
