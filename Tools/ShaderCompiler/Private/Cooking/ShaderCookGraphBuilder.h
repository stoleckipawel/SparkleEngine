#pragma once

#include "Cooking/ShaderCookContext.h"

#include <cstddef>
#include <string>

class ShaderBackendPool;
struct ShaderPackageCookSettings;

class ShaderCookGraphBuilder final
{
  public:
	ShaderCookGraphBuilder() = delete;

	static bool Build(
	    const ShaderPackageCookSettings& settings,
	    bool writeDebugArtifacts,
	    ShaderBackendPool& backendPool,
	    ShaderCookPipelinePlan& outPlan,
	    std::string& outErrorMessage);

  private:
	static bool AddPackageNodes(
	    const ShaderPackageCookSettings& settings,
	    bool writeDebugArtifacts,
	    std::size_t packageIndex,
	    ShaderBackendPool& backendPool,
	    ShaderCookPipelinePlan& plan,
	    std::string& outErrorMessage);
};