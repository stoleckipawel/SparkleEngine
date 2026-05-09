#include "AssetCookerDiagnostics.h"

#include <utility>

void AssetCookerDiagnostics::AddInfo(AssetCookerCategory category, std::string message)
{
	Add(AssetCookerDiagnosticSeverity_Info, category, std::move(message), std::string());
}

void AssetCookerDiagnostics::AddWarning(AssetCookerCategory category, std::string message)
{
	Add(AssetCookerDiagnosticSeverity_Warning, category, std::move(message), std::string());
}

void AssetCookerDiagnostics::AddError(AssetCookerCategory category, std::string message)
{
	Add(AssetCookerDiagnosticSeverity_Error, category, std::move(message), std::string());
}

void AssetCookerDiagnostics::AddError(
    AssetCookerCategory category,
    std::string message,
    const std::filesystem::path& sourcePath)
{
	Add(AssetCookerDiagnosticSeverity_Error, category, std::move(message), sourcePath.string());
}

const std::vector<AssetCookerDiagnosticRecord>& AssetCookerDiagnostics::GetRecords() const noexcept
{
	return records;
}

std::vector<AssetCookerDiagnosticRecord> AssetCookerDiagnostics::ReleaseRecords()
{
	return std::move(records);
}

void AssetCookerDiagnostics::Add(
    AssetCookerDiagnosticSeverity severity,
    AssetCookerCategory category,
    std::string message,
    std::string sourcePath)
{
	AssetCookerDiagnosticRecord record;
	record.severity = severity;
	record.category = category;
	record.message = std::move(message);
	record.sourcePath = std::move(sourcePath);
	records.push_back(std::move(record));
}
