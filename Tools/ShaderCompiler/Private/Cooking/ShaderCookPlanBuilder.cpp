#include "PCH.h"

#include "Cooking/ShaderCookPlanBuilder.h"

#include "Backend/ShaderBackendPool.h"
#include "Cooking/ShaderCookNodeBuilder.h"
#include "Cooking/ShaderCookPlanner.h"
#include "Cooking/ShaderPackageCooker.h"

bool ShaderCookPlanBuilder::Build(
    const ShaderPackageCookSettings& settings,
    bool writeDebugArtifacts,
    ShaderBackendPool& backendPool,
    ShaderCookPipelinePlan& outPlan,
    std::string& outErrorMessage)
{
	outPlan = {};
	outPlan.packages = ShaderCookPlanner::BuildPackages(settings, outErrorMessage);
	if (!outErrorMessage.empty())
	{
		return false;
	}

	outPlan.packageContexts.resize(outPlan.packages.size());
	for (std::size_t packageIndex = 0; packageIndex < outPlan.packages.size(); ++packageIndex)
	{
		if (!AddPackageNodes(settings, writeDebugArtifacts, packageIndex, backendPool, outPlan, outErrorMessage))
		{
			return false;
		}
	}

	outErrorMessage.clear();
	return true;
}

bool ShaderCookPlanBuilder::AddPackageNodes(
    const ShaderPackageCookSettings& settings,
    bool writeDebugArtifacts,
    std::size_t packageIndex,
    ShaderBackendPool& backendPool,
    ShaderCookPipelinePlan& plan,
    std::string& outErrorMessage)
{
	const ShaderCookPackageDesc& package = plan.packages[packageIndex];
	ShaderCookPackageContext& packageContext = plan.packageContexts[packageIndex];
	packageContext.compiledStages.reserve(package.stages.size() * settings.targets.size());

	for (std::size_t stageIndex = 0; stageIndex < package.stages.size(); ++stageIndex)
	{
		for (std::size_t targetIndex = 0; targetIndex < settings.targets.size(); ++targetIndex)
		{
			if (!ShaderCookNodeBuilder::BuildAndAdd(
			        settings,
			        writeDebugArtifacts,
			        packageIndex,
			        stageIndex,
			        targetIndex,
			        backendPool,
			        plan,
			        outErrorMessage))
			{
				return false;
			}
		}
	}

	outErrorMessage.clear();
	return true;
}