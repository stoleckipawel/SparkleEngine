#pragma once

#include "SourceImportOutput.h"

#include <filesystem>
#include <string_view>

class SourceImporter
{
public:
	virtual ~SourceImporter();
	SourceImporter(const SourceImporter&) = delete;
	SourceImporter& operator=(const SourceImporter&) = delete;
	SourceImporter(SourceImporter&&) = delete;
	SourceImporter& operator=(SourceImporter&&) = delete;

	virtual std::string_view GetImporterId() const noexcept = 0;
	virtual bool SupportsExtension(std::wstring_view extension) const noexcept = 0;
	virtual SourceImportOutput Import(const std::filesystem::path& filePath) const = 0;

protected:
	SourceImporter() = default;
};
