#pragma once

#include "Cooking/ShaderCookContext.h"

#include <cstddef>
#include <string>

class IShaderArtifactStore;
class ShaderBackendPool;
struct ShaderDebugArtifactSet;
struct ShaderPackageCookSettings;

struct ShaderCookExecutionCounters final
{
	std::size_t backendInvocationCount = 0;
	std::size_t cacheHitCount = 0;
	std::size_t cacheMissCount = 0;
	std::size_t processedNodeCount = 0;
};

class ShaderCookGraphExecutor final
{
  public:
	ShaderCookGraphExecutor() = delete;

	static bool Execute(
	    const ShaderPackageCookSettings& settings,
	    bool writeDebugArtifacts,
	    ShaderCookPipelinePlan& plan,
	    ShaderBackendPool& backendPool,
	    IShaderArtifactStore& artifactStore,
	    ShaderCookExecutionCounters& counters,
	    std::string& outErrorMessage);

  private:
	static bool ExecuteNode(
	    const ShaderPackageCookSettings& settings,
	    bool writeDebugArtifacts,
	    const CookNode& node,
	    ShaderCookPipelinePlan& plan,
	    ShaderBackendPool& backendPool,
	    IShaderArtifactStore& artifactStore,
	    ShaderCookExecutionCounters& counters,
	    std::string& outErrorMessage);

	static bool VerifyParameterStruct(
	    const ShaderPackageCookSettings& settings,
	    const CookNode& node,
	    const CookedStageBuild& compiledStage,
	    ShaderDebugArtifactSet* debugArtifacts,
	    std::string& outErrorMessage);
};