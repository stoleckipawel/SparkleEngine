#include "PCH.h"

#include "Cooking/ShaderCookPlanBuilder.h"

#include "Backend/IShaderBackend.h"
#include "Backend/ShaderBackendPool.h"
#include "Cooking/ShaderCookNodeBuilder.h"
#include "Cooking/ShaderCookPlanner.h"
#include "Cooking/ShaderPackageCooker.h"

#include <format>

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
	const std::size_t packageNodeStart = plan.nodes.size();

	for (std::size_t targetIndex = 0; targetIndex < settings.targets.size(); ++targetIndex)
	{
		bool shouldCookTarget = false;
		if (!ShouldCookPackageTarget(
		        settings,
		        package,
		        settings.targets[targetIndex],
		        backendPool,
		        shouldCookTarget,
		        outErrorMessage))
		{
			return false;
		}

		if (!shouldCookTarget)
		{
			continue;
		}

		for (std::size_t stageIndex = 0; stageIndex < package.stages.size(); ++stageIndex)
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

	if (plan.nodes.size() == packageNodeStart)
	{
		outErrorMessage = std::format(
		    "No supported shader targets were available for shader package '{}' from the requested target set",
		    package.packageId);
		return false;
	}

	outErrorMessage.clear();
	return true;
}

bool ShaderCookPlanBuilder::ShouldCookPackageTarget(
    const ShaderPackageCookSettings& settings,
    const ShaderCookPackageDesc& package,
    ShaderTarget target,
    ShaderBackendPool& backendPool,
    bool& outShouldCook,
    std::string& outErrorMessage)
{
	outShouldCook = false;
	if (package.stages.empty())
	{
		outErrorMessage = std::format("Shader package '{}' has no registered stages", package.packageId);
		return false;
	}

	ShaderCompileOptions compileOptions = ShaderCookPlanner::BuildCompileOptions(package.stages.front());
	compileOptions.Target = target;

	std::string backendError;
	std::string backendName;
	IShaderBackend* backend = backendPool.ResolveAndAcquire(
	    compileOptions.SourcePath,
	    compileOptions.Target,
	    settings.backendName,
	    backendName,
	    backendError);
	if (backendName.empty())
	{
		outErrorMessage = std::format(
		    "Failed to select shader backend for shader package '{}' target '{}' - {}",
		    package.packageId,
		    GetShaderTargetName(target),
		    backendError);
		return false;
	}
	if (backend == nullptr)
	{
		outErrorMessage = std::format(
		    "Failed to construct shader backend '{}' for shader package '{}' target '{}' - {}",
		    backendName,
		    package.packageId,
		    GetShaderTargetName(target),
		    backendError);
		return false;
	}

	const ShaderBackendCapabilities capabilities = backend->GetCapabilities();
	if (package.packageKind == CookedShaderPackageKind::RayTracingLibrary &&
	    !capabilities.SupportsRayTracingLibrary(target))
	{
		outErrorMessage.clear();
		return true;
	}

	if (HasCookedShaderPackageFeature(package.packageFeatures, CookedShaderPackageFeatureFlags::UsesInlineRayQuery) &&
	    !capabilities.SupportsInlineRayQuery(target))
	{
		outErrorMessage.clear();
		return true;
	}

	outShouldCook = true;
	outErrorMessage.clear();
	return true;
}