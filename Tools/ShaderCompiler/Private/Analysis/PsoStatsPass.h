#pragma once

#include "Cooking/CookedShaderPackageOutput.h"

#include <filesystem>
#include <span>
#include <string>

struct PsoStatsPassResult final
{
	std::filesystem::path outputPath;
	std::uint32_t rowCount = 0;
};

class PsoStatsPass final
{
  public:
	PsoStatsPass() = delete;

	static bool WriteCsv(
	    std::span<const CookedShaderPackageOutput> packages,
	    const std::filesystem::path& analysisDirectory,
	    PsoStatsPassResult& outResult,
	    std::string& outErrorMessage);
};