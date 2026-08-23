#include "PCH.h"
#include "Core/Public/FileSystemUtils.h"

#include "Cooking/ShaderPackageCooker.h"

#include "Backend/ShaderBackendPool.h"
#include "Cooking/CookedShaderPackageEmitter.h"
#include "Cooking/ShaderCookPlanExecutor.h"
#include "Cooking/ShaderCookPlanBuilder.h"
#include "Core/Public/Diagnostics/Error.h"
#include <vector>

ShaderPackageCookResult ShaderPackageCooker::CookAll(const ShaderPackageCookSettings& settings) const
{
	ShaderPackageCookResult result;
	result.cacheDirectory = settings.cacheDirectory.empty() ? Filesystem::GetShaderCacheRootPath() : settings.cacheDirectory;
	ShaderBackendPool backendPool;
	ShaderCookPipelinePlan plan = ShaderCookPlanBuilder::Build(settings, backendPool);

	std::vector<CookedStageBuild> compiledStages =
	    ShaderCookPlanExecutor::Execute(settings, plan, result.cacheDirectory);
	for (std::size_t index = 0; index < compiledStages.size(); ++index)
	{
		plan.packageContexts[plan.nodes[index].packageIndex].push_back(std::move(compiledStages[index]));
	}

	result.packages = CookedShaderPackageEmitter::Emit(plan, result.cacheDirectory);

	return result;
}
