#include "AssetCookerDiagnostics.h"

#include <iterator>
#include <utility>

void AssetCookerDiagnostics::AddError(AssetCookerCategory category, std::string message)
{
	Add(category, std::move(message), std::string());
}

void AssetCookerDiagnostics::AddError(
    AssetCookerCategory category,
    std::string message,
    const std::filesystem::path& sourcePath)
{
	Add(category, std::move(message), sourcePath.string());
}

void AssetCookerDiagnostics::Append(std::vector<AssetCookerDiagnosticRecord> additionalRecords)
{
	records.insert(
	    records.end(),
	    std::make_move_iterator(additionalRecords.begin()),
	    std::make_move_iterator(additionalRecords.end()));
}

std::vector<AssetCookerDiagnosticRecord> AssetCookerDiagnostics::ReleaseRecords()
{
	return std::move(records);
}

void AssetCookerDiagnostics::Add(
    AssetCookerCategory category,
    std::string message,
    std::string sourcePath)
{
	AssetCookerDiagnosticRecord record;
	record.category = category;
	record.message = std::move(message);
	record.sourcePath = std::move(sourcePath);
	records.push_back(std::move(record));
}
