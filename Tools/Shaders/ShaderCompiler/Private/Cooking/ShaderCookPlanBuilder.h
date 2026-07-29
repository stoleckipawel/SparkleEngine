#pragma once

#include "Cooking/ShaderCookContext.h"

#include <cstddef>

class ShaderBackendPool;
struct ShaderPackageCookSettings;

class ShaderCookPlanBuilder final
{
  public:
	ShaderCookPlanBuilder() = delete;

	static ShaderCookPipelinePlan Build(
	    const ShaderPackageCookSettings& settings,
	    ShaderBackendPool& backendPool);

  private:
	static void AddPackageNodes(
	    const ShaderPackageCookSettings& settings,
	    std::size_t packageIndex,
	    ShaderBackendPool& backendPool,
	    ShaderCookPipelinePlan& plan);

	static bool ShouldCookPackageTarget(
	    const ShaderPackageCookSettings& settings,
	    const ShaderCookPackageDesc& package,
	    ShaderTarget target,
	    ShaderBackendPool& backendPool);
};
