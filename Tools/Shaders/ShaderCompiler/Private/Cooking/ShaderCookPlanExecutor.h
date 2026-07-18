#pragma once

#include "Cooking/ShaderCookNodeResult.h"

#include <filesystem>
#include <string>
#include <vector>

struct ShaderCookPipelinePlan;
struct ShaderPackageCookSettings;

class ShaderCookPlanExecutor final
{
  public:
	ShaderCookPlanExecutor() = delete;

	static bool Execute(
	    const ShaderPackageCookSettings& settings,
	    const ShaderCookPipelinePlan& plan,
	    const std::filesystem::path& cacheDirectory,
	    std::vector<ShaderCookNodeResult>& outNodeResults,
	    std::string& outErrorMessage);
};
