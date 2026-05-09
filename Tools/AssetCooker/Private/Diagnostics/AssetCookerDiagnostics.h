#pragma once

#include "../../Public/AssetCookerTypes.h"

#include <filesystem>
#include <string>
#include <vector>

struct AssetCookerDiagnosticRecord final
{
	AssetCookerDiagnosticSeverity severity = AssetCookerDiagnosticSeverity_Info;
	AssetCookerCategory category = AssetCookerCategory_All;
	std::string message;
	std::string sourcePath;
};

struct AssetCookerOutputRecord final
{
	AssetCookerCategory category = AssetCookerCategory_All;
	std::string assetId;
	std::string path;
	std::string reloadHint;
	std::uint32_t version = 1;
};

struct AssetCookerServiceResult final
{
	bool succeeded = false;
	int exitCode = 1;
	std::vector<AssetCookerDiagnosticRecord> diagnostics;
	std::vector<AssetCookerOutputRecord> outputs;
};

class AssetCookerDiagnostics final
{
public:
	void AddInfo(AssetCookerCategory category, std::string message);
	void AddWarning(AssetCookerCategory category, std::string message);
	void AddError(AssetCookerCategory category, std::string message);
	void AddError(AssetCookerCategory category, std::string message, const std::filesystem::path& sourcePath);

	const std::vector<AssetCookerDiagnosticRecord>& GetRecords() const noexcept;
	std::vector<AssetCookerDiagnosticRecord> ReleaseRecords();

private:
	void Add(
	    AssetCookerDiagnosticSeverity severity,
	    AssetCookerCategory category,
	    std::string message,
	    std::string sourcePath);

	std::vector<AssetCookerDiagnosticRecord> records;
};
