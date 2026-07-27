#include "PCH.h"
#include "Core/Public/FileSystemUtils.h"

#include "Cooking/ShaderPackageCooker.h"

#include "Backend/ShaderBackendPool.h"
#include "Cooking/CookedShaderPackageEmitter.h"
#include "Cooking/ShaderCookPlanExecutor.h"
#include "Cooking/ShaderCookPlanBuilder.h"
#include "Cooking/ShaderCookNodeResult.h"
#include <vector>

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

	std::vector<ShaderCookNodeResult> nodeResults;
	if (!ShaderCookPlanExecutor::Execute(settings, plan, result.cacheDirectory, nodeResults, result.errorMessage))
	{
		result.packages.clear();
		return result;
	}
	for (std::size_t index = 0; index < nodeResults.size(); ++index)
	{
		ShaderCookNodeResult& nodeResult = nodeResults[index];
		plan.packageContexts[plan.nodes[index].packageIndex].compiledStages.push_back(std::move(nodeResult.CompiledStage));
	}

	if (!CookedShaderPackageEmitter::Emit(plan, result.cacheDirectory, result, result.errorMessage))
	{
		return result;
	}

	return result;
}
