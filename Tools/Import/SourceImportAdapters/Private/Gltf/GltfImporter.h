#pragma once

#include "SourceImportResult.h"
#include "SourceImporter.h"

#include <filesystem>
#include <string_view>

class GltfImporter final : public SourceImporter
{
  public:
	bool SupportsExtension(std::wstring_view extension) const noexcept override;
	SourceImportResult Import(const std::filesystem::path& filePath) const override;
};


