#pragma once

#include "Cooking/ShaderCookOutput.h"

#include <cstdint>
#include <filesystem>

struct CookedShaderStatsReport final
{
	std::filesystem::path outputPath;
	std::uint32_t rowCount = 0;
};

class CookedShaderStatsPass final
{
public:
	CookedShaderStatsPass() = delete;

	static CookedShaderStatsReport WriteCsv(const ShaderCookOutput& output, const std::filesystem::path& analysisDirectory);
};
