#include "PCH.h"

#include "Cooking/ShaderPackageCooker.h"

#include "Backend/ShaderBackendPool.h"
#include "Cooking/Cache/LocalDiskShaderArtifactStore.h"
#include "Cooking/CookedShaderPackageEmitter.h"
#include "Cooking/ShaderCookPlanBuilder.h"
#include "Cooking/ShaderCookPlanExecutor.h"
#include "Core/Public/Paths/DirectoryPaths.h"

ShaderPackageCookResult ShaderPackageCooker::CookAll(const ShaderPackageCookSettings& settings) const
{
	ShaderPackageCookResult result;
	result.cacheDirectory = settings.cacheDirectory.empty() ? Paths::ShaderCacheRoot() : settings.cacheDirectory;
	const bool writeDebugArtifacts = !settings.debugArtifactDirectory.empty();
	ShaderBackendPool backendPool;
	ShaderCookPipelinePlan plan;
	if (!ShaderCookPlanBuilder::Build(settings, writeDebugArtifacts, backendPool, plan, result.errorMessage))
	{
		return result;
	}

	LocalDiskShaderArtifactStore artifactStore(result.cacheDirectory);
	ShaderCookExecutionCounters counters;
	if (!ShaderCookPlanExecutor::Execute(
	        settings,
	        writeDebugArtifacts,
	        plan,
	        backendPool,
	        artifactStore,
	        counters,
	        result.errorMessage))
	{
		result.packages.clear();
		return result;
	}
	result.backendInvocationCount = counters.backendInvocationCount;
	result.cacheHitCount = counters.cacheHitCount;
	result.cacheMissCount = counters.cacheMissCount;

	if (!CookedShaderPackageEmitter::Emit(plan, result.cacheDirectory, result, result.errorMessage))
	{
		return result;
	}

	return result;
}

