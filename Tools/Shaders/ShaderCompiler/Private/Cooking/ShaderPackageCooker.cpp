#include "PCH.h"
#include "Core/Public/FileSystemUtils.h"

#include "Cooking/ShaderPackageCooker.h"

#include "Backend/ShaderBackendPool.h"
#include "Cooking/Cache/LocalDiskShaderArtifactStore.h"
#include "Cooking/CookedShaderPackageEmitter.h"
#include "Cooking/ShaderCookNodeExecutor.h"
#include "Cooking/ShaderCookPlanBuilder.h"
#include "Cooking/ShaderCookProgressReporter.h"

ShaderPackageCookResult ShaderPackageCooker::CookAll(const ShaderPackageCookSettings& settings) const
{
	ShaderPackageCookResult result;
	result.cacheDirectory = settings.cacheDirectory.empty() ? Filesystem::GetShaderCacheRootPath() : settings.cacheDirectory;
	ShaderBackendPool backendPool;
	ShaderCookPipelinePlan plan;
	if (!ShaderCookPlanBuilder::Build(settings, backendPool, plan, result.errorMessage))
	{
		return result;
	}

	LocalDiskShaderArtifactStore artifactStore(result.cacheDirectory);
	ShaderCookExecutionCounters counters;
	ShaderCookProgressReporter::PrintPlanSummary(plan, settings);
	for (const CookNode& node : plan.nodes)
	{
		if (!ShaderCookNodeExecutor::Execute(
		        settings,
		        node,
		        plan,
		        backendPool,
		        artifactStore,
		        counters,
		        result.errorMessage))
		{
			result.packages.clear();
			return result;
		}
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
