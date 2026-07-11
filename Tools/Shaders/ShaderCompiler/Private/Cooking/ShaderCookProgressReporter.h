#pragma once

#include "Cooking/ShaderCookContext.h"
#include "Cooking/ShaderCookExecutionCounters.h"

#include <cstddef>
#include <string_view>

struct ShaderPackageCookSettings;

class ShaderCookProgressReporter final
{
  public:
	ShaderCookProgressReporter() = delete;

	static void PrintPlanSummary(const ShaderCookPipelinePlan& plan, const ShaderPackageCookSettings& settings);
	static void PrintPackageProgress(
	    const ShaderCookPipelinePlan& plan,
	    const CookNode& node);
	static void PrintStageProgress(
	    const ShaderCookPipelinePlan& plan,
	    const ShaderCookExecutionCounters& counters,
	    const CookNode& node,
	    std::string_view backendName,
	    std::string_view status);
};
