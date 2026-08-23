#include "PCH.h"

#include "Cooking/ShaderCookPlanBuilder.h"

#include "Backend/IShaderBackend.h"
#include "Backend/ShaderBackendPool.h"
#include "Cooking/ShaderCookNodeBuilder.h"
#include "Cooking/ShaderCookPlanner.h"
#include "Cooking/ShaderCookSettings.h"

#include "Core/Public/Diagnostics/Error.h"

#include <format>

ShaderCookPipelinePlan ShaderCookPlanBuilder::Build(
    const ShaderPackageCookSettings& settings,
    ShaderBackendPool& backendPool)
{
	ShaderCookPipelinePlan plan;
	plan.packages = ShaderCookPlanner::BuildPackages(settings);

	plan.packageContexts.resize(plan.packages.size());
	for (std::size_t packageIndex = 0; packageIndex < plan.packages.size(); ++packageIndex)
	{
		AddPackageNodes(settings, packageIndex, backendPool, plan);
	}

	return plan;
}


void ShaderCookPlanBuilder::AddPackageNodes(
    const ShaderPackageCookSettings& settings,
    std::size_t packageIndex,
    ShaderBackendPool& backendPool,
    ShaderCookPipelinePlan& plan)
{
	const ShaderCookPackageDesc& package = plan.packages[packageIndex];
	ShaderCookPackageContext& packageContext = plan.packageContexts[packageIndex];
	packageContext.reserve(package.stages.size() * settings.targets.size());
	const std::size_t packageNodeStart = plan.nodes.size();

	for (std::size_t targetIndex = 0; targetIndex < settings.targets.size(); ++targetIndex)
	{
		if (!ShouldCookPackageTarget(settings, package, settings.targets[targetIndex], backendPool))
		{
			continue;
		}

		for (std::size_t stageIndex = 0; stageIndex < package.stages.size(); ++stageIndex)
		{
			ShaderCookNodeBuilder::BuildAndAdd(
			    settings,
			    packageIndex,
			    stageIndex,
			    targetIndex,
			    backendPool,
			    plan);
		}
	}

	if (plan.nodes.size() == packageNodeStart)
	{
		throw Diagnostics::Error(std::format(
		    "No supported shader targets were available for shader package '{}' from the requested target set",
		    package.packageId));
	}
}

bool ShaderCookPlanBuilder::ShouldCookPackageTarget(
    const ShaderPackageCookSettings& settings,
    const ShaderCookPackageDesc& package,
    ShaderTarget target,
    ShaderBackendPool& backendPool)
{
	if (package.stages.empty())
	{
		throw Diagnostics::Error(std::format("Shader package '{}' has no registered stages.", package.packageId));
	}

	ShaderCompileOptions compileOptions = ShaderCookPlanner::BuildCompileOptions(package.stages.front());
	compileOptions.Target = target;

	IShaderBackend& backend = backendPool.ResolveAndAcquire(
	    compileOptions.SourcePath,
	    compileOptions.Target,
	    settings.backendName);

	const ShaderBackendCapabilities capabilities = backend.GetCapabilities();
	if (package.packageKind == CookedShaderPackageKind::RayTracingLibrary &&
	    !capabilities.SupportsRayTracingLibrary(target))
	{
		return false;
	}

	if (HasCookedShaderPackageFeature(package.packageFeatures, CookedShaderPackageFeatureFlags::UsesInlineRayQuery) &&
	    !capabilities.SupportsInlineRayQuery(target))
	{
		return false;
	}

	return true;
}
