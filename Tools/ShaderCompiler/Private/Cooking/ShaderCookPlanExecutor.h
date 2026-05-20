#pragma once

#include "Cooking/ShaderCookContext.h"
#include "Cooking/ShaderCookExecutionCounters.h"

#include <string>

class IShaderArtifactStore;
class ShaderBackendPool;
struct ShaderPackageCookSettings;

class ShaderCookPlanExecutor final
{
  public:
	ShaderCookPlanExecutor() = delete;

	static bool Execute(
	    const ShaderPackageCookSettings& settings,
	    bool writeDebugArtifacts,
	    ShaderCookPipelinePlan& plan,
	    ShaderBackendPool& backendPool,
	    IShaderArtifactStore& artifactStore,
	    ShaderCookExecutionCounters& counters,
	    std::string& outErrorMessage);
};