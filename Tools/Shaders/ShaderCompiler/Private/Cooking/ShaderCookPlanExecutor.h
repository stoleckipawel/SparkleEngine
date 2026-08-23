#pragma once

#include "Cooking/CookedStageBuild.h"

#include <vector>

struct ShaderCookPipelinePlan;
struct ShaderPackageCookSettings;

class ShaderCookPlanExecutor final
{
  public:
	ShaderCookPlanExecutor() = delete;

	static std::vector<CookedStageBuild> Execute(
	    const ShaderPackageCookSettings& settings,
	    const ShaderCookPipelinePlan& plan);
};
