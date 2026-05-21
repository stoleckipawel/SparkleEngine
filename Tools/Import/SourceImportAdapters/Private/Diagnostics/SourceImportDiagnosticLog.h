#pragma once

#include <filesystem>
#include <string_view>

struct SourceImportResult;

class SourceImportDiagnosticLog final
{
  public:
	static void ReportUnsupportedExtension(std::wstring_view extension, const std::filesystem::path& filePath, SourceImportResult& result);
};