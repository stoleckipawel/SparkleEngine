#pragma once

#include "Cooking/CookedShaderPackageOutput.h"

#include <filesystem>
#include <span>
#include <string>

struct CookedShaderStatsReport final
{
	std::filesystem::path outputPath;
	std::uint32_t rowCount = 0;
};

class CookedShaderStatsPass final
{
  public:
	CookedShaderStatsPass() = delete;

	static CookedShaderStatsReport WriteCsv(
	    std::span<const CookedShaderPackageOutput> packages,
	    const std::filesystem::path& analysisDirectory);
};
