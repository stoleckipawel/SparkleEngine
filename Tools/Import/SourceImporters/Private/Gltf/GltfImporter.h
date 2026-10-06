#pragma once

#include "SourceImportOutput.h"

#include <filesystem>

SourceImportOutput ImportGltfScene(const std::filesystem::path& filePath);
