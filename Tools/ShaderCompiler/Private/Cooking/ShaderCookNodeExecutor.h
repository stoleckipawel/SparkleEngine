#pragma once

#include "Cooking/ShaderCookContext.h"
#include "Cooking/ShaderCookExecutionCounters.h"

#include <string>

class IShaderArtifactStore;
class IShaderBackend;
class ShaderBackendPool;
struct CookedStageBuild;
struct ShaderDebugArtifactSet;
struct ShaderCacheKey;
struct ShaderPackageCookSettings;

class ShaderCookNodeExecutor final
{
  public:
	ShaderCookNodeExecutor() = delete;

	static bool Execute(
	    const ShaderPackageCookSettings& settings,
	    bool writeDebugArtifacts,
	    const CookNode& node,
	    ShaderCookPipelinePlan& plan,
	    ShaderBackendPool& backendPool,
	    IShaderArtifactStore& artifactStore,
	    ShaderCookExecutionCounters& counters,
	    std::string& outErrorMessage);

  private:
	static bool TryCompleteFromCache(
	    const ShaderPackageCookSettings& settings,
	    bool writeDebugArtifacts,
	    const CookNode& node,
	    IShaderBackend& backend,
	    ShaderCookPipelinePlan& plan,
	    IShaderArtifactStore& artifactStore,
	    ShaderCookExecutionCounters& counters,
	    bool& outCompleted,
	    std::string& outErrorMessage);

	static bool CompileStage(
	    IShaderBackend& backend,
	    const CookNode& node,
	    bool writeDebugArtifacts,
	    CookedStageBuild& compiledStage,
	    ShaderDebugArtifactSet& debugArtifacts,
	    std::string& outErrorMessage);

	static bool WriteDebugArtifacts(
	    const ShaderPackageCookSettings& settings,
	    const CookNode& node,
	    const CookedStageBuild& compiledStage,
	    const ShaderDebugArtifactSet& debugArtifacts,
	    std::string& outErrorMessage);

	static bool StoreCompiledStage(
	    const ShaderPackageCookSettings& settings,
	    const ShaderCacheKey& cacheKey,
	    const CookedStageBuild& compiledStage,
	    IShaderArtifactStore& artifactStore,
	    std::string& outErrorMessage);

	static void RecordCompletedStage(
	    const CookNode& node,
	    CookedStageBuild&& compiledStage,
	    ShaderCookPipelinePlan& plan,
	    ShaderCookExecutionCounters& counters);
};