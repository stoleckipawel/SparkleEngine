#pragma once

#include "SourceImportOutput.h"
#include "SourceImporter.h"

#include <filesystem>
#include <string_view>

class GltfImporter final : public SourceImporter
{
public:
	std::string_view GetImporterId() const noexcept override;
	bool SupportsExtension(std::wstring_view extension) const noexcept override;
	SourceImportOutput Import(const std::filesystem::path& filePath) const override;
};
