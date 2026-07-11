#pragma once

#include "Cooking/ShaderCookContext.h"

#include <cstddef>
#include <string>

class ShaderBackendPool;
struct ShaderPackageCookSettings;

class ShaderCookPlanBuilder final
{
  public:
	ShaderCookPlanBuilder() = delete;

	static bool Build(
	    const ShaderPackageCookSettings& settings,
	    ShaderBackendPool& backendPool,
	    ShaderCookPipelinePlan& outPlan,
	    std::string& outErrorMessage);

  private:
	static bool AddPackageNodes(
	    const ShaderPackageCookSettings& settings,
	    std::size_t packageIndex,
	    ShaderBackendPool& backendPool,
	    ShaderCookPipelinePlan& plan,
	    std::string& outErrorMessage);

	static bool ShouldCookPackageTarget(
	    const ShaderPackageCookSettings& settings,
	    const ShaderCookPackageDesc& package,
	    ShaderTarget target,
	    ShaderBackendPool& backendPool,
	    bool& outShouldCook,
	    std::string& outErrorMessage);
};
