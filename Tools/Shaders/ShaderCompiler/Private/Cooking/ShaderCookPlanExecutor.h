#pragma once

#include "Cooking/CookedStageBuild.h"

#include <filesystem>
#include <vector>

struct ShaderCookPipelinePlan;
struct ShaderPackageCookSettings;

class ShaderCookPlanExecutor final
{
  public:
	ShaderCookPlanExecutor() = delete;

	static std::vector<CookedStageBuild> Execute(
	    const ShaderPackageCookSettings& settings,
	    const ShaderCookPipelinePlan& plan,
	    const std::filesystem::path& cacheDirectory);
};
