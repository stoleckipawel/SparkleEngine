#pragma once

#include "SourceImporter.h"

class PlyImporter final : public SourceImporter
{
public:
	std::string_view GetImporterId() const noexcept override;
	bool SupportsExtension(std::wstring_view extension) const noexcept override;
	SourceImportOutput Import(const std::filesystem::path& filePath) const override;
};
