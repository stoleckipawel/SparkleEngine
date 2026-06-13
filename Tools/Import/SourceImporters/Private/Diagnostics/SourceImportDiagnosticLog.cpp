#include "PCH.h"

#include "Diagnostics/SourceImportDiagnosticLog.h"

#include "Core/Public/Strings/StringUtils.h"
#include "Diagnostics/SourceImportDiagnosticsRecorder.h"

#include <format>

static const auto g_sourceImportDiagnosticLogger = Logging::GetOrCreateLogger("Tools.SourceImporters");

void SourceImportDiagnosticLog::ReportUnsupportedExtension(
    std::wstring_view extension,
    const std::filesystem::path& filePath,
    SourceImportResult& result)
{
	SPDLOG_LOGGER_ERROR(
	    g_sourceImportDiagnosticLogger,
	    "{}",
	    std::format(
	        "SourceSceneImporter: Unsupported asset extension '{}' for '{}'",
	        extension.empty() ? std::string("<none>") : Strings::ToNarrow(extension),
	        filePath.string()));
	SourceImportDiagnosticsRecorder::RecordError(result);
}
