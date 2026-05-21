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
	    const ShaderPackageCookSettings& settings,
	    const CookNode& node);
	static void PrintStageProgress(
	    const ShaderCookPipelinePlan& plan,
	    const ShaderCookExecutionCounters& counters,
	    const CookNode& node,
	    std::string_view backendName,
	    std::string_view status);

  private:
	static std::size_t CountPackageJobs(const ShaderCookPackageDesc& package, const ShaderPackageCookSettings& settings) noexcept;
};