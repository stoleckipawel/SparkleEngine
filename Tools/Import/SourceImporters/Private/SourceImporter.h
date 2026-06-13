#pragma once

#include "SourceImportResult.h"

#include <filesystem>
#include <string_view>

class SourceImporter
{
  public:
	virtual ~SourceImporter();

	virtual std::string_view GetImporterId() const noexcept = 0;
	virtual bool SupportsExtension(std::wstring_view extension) const noexcept = 0;
	virtual SourceImportResult Import(const std::filesystem::path& filePath) const = 0;
};


