#include "PCH.h"

#include "Cooking/ShaderPackageCooker.h"

#include "Backend/ShaderBackendPool.h"
#include "Cooking/Cache/LocalDiskShaderArtifactStore.h"
#include "Cooking/CookedShaderPackageEmitter.h"
#include "Cooking/ShaderCookGraphBuilder.h"
#include "Cooking/ShaderCookGraphExecutor.h"
#include "Core/Public/FileSystemUtils.h"
#include "Core/Public/Paths/PathUtils.h"

std::filesystem::path ShaderPackageCooker::ResolveCacheDirectory(const ShaderPackageCookSettings& settings)
{
	if (!settings.cacheDirectory.empty())
	{
		return settings.cacheDirectory;
	}

	return Engine::Paths::Normalize(Filesystem::GetExecutableDirectory().parent_path() / "Cache" / "Shaders");
}
ShaderPackageCookResult ShaderPackageCooker::CookAll(const ShaderPackageCookSettings& settings) const
{
	ShaderPackageCookResult result;
	result.cacheDirectory = ResolveCacheDirectory(settings);
	const bool writeDebugArtifacts = !settings.debugArtifactDirectory.empty();
	ShaderBackendPool backendPool;
	ShaderCookPipelinePlan plan;
	if (!ShaderCookGraphBuilder::Build(settings, writeDebugArtifacts, backendPool, plan, result.errorMessage))
	{
		return result;
	}

	LocalDiskShaderArtifactStore artifactStore(result.cacheDirectory);
	ShaderCookExecutionCounters counters;
	if (!ShaderCookGraphExecutor::Execute(
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

