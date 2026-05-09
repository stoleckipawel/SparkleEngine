#pragma once

#include "AssetCookerTypes.h"

#include <cstdint>

struct AssetCookDiagnostic
{
	AssetCookerDiagnosticSeverity severity;
	AssetCookerCategory category;
	const char* message;
	const char* sourcePath;
};

struct AssetCookedOutput
{
	AssetCookerCategory category;
	const char* assetId;
	const char* path;
	const char* reloadHint;
	std::uint32_t version;
};

struct AssetCookResult
{
	std::uint32_t succeeded;
	int exitCode;
	const AssetCookedOutput* outputs;
	std::uint32_t outputCount;
	const AssetCookDiagnostic* diagnostics;
	std::uint32_t diagnosticCount;
};
