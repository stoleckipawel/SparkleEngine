#include "PCH.h"

#include "Cooking/ShaderCookGraphBuilder.h"

#include "Backend/IShaderBackend.h"
#include "Backend/ShaderBackendPool.h"
#include "Cooking/Cache/IncludeClosureHasher.h"
#include "Cooking/Cache/ShaderCacheKey.h"
#include "Cooking/Cache/ShaderCompileOptionsHasher.h"
#include "Cooking/ShaderCookPlanner.h"
#include "Cooking/ShaderPackageCooker.h"

#include <format>

bool ShaderCookGraphBuilder::Build(
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

bool ShaderCookGraphBuilder::AddPackageNodes(
    const ShaderPackageCookSettings& settings,
    bool writeDebugArtifacts,
    std::size_t packageIndex,
    ShaderBackendPool& backendPool,
    ShaderCookPipelinePlan& plan,
    std::string& outErrorMessage)
{
	const ShaderCookPackageDesc& package = plan.packages[packageIndex];
	ShaderCookPackageContext& packageContext = plan.packageContexts[packageIndex];
	packageContext.compiledStages.reserve(package.stages.size());

	for (std::size_t stageIndex = 0; stageIndex < package.stages.size(); ++stageIndex)
	{
		const ShaderCookStageDesc& stage = package.stages[stageIndex];
		ShaderCompileOptions compileOptions = ShaderCookPlanner::BuildCompileOptions(stage);
		compileOptions.Target = settings.target;
		compileOptions.CaptureDebugArtifacts = writeDebugArtifacts;

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
			    "Failed to select shader backend for shader package '{}' variant '{}' stage '{}' - {}",
			    package.packageId,
			    package.variantId,
			    GetShaderStagePrefix(stage.stage),
			    backendError);
			return false;
		}
		if (backend == nullptr)
		{
			outErrorMessage = std::format(
			    "Failed to construct shader backend '{}' for shader package '{}' variant '{}' stage '{}' - {}",
			    backendName,
			    package.packageId,
			    package.variantId,
			    GetShaderStagePrefix(stage.stage),
			    backendError);
			return false;
		}

		const IncludeClosureHashResult includeHashResult = IncludeClosureHasher::Compute(compileOptions);
		if (!includeHashResult.Succeeded())
		{
			outErrorMessage = std::format(
			    "Failed to compute include closure for shader package '{}' variant '{}' stage '{}' - {}",
			    package.packageId,
			    package.variantId,
			    GetShaderStagePrefix(stage.stage),
			    includeHashResult.errorMessage);
			return false;
		}

		const std::uint64_t optionsHash = ShaderCompileOptionsHasher::Compute(compileOptions);
		plan.graph.AddNode(CookNode{
		    .packageIndex = packageIndex,
		    .stageIndex = stageIndex,
		    .package = &package,
		    .stage = &stage,
		    .backendName = backendName,
		    .compileOptions = compileOptions,
		    .parameterStructDescriptor = ShaderCookPlanner::FindParameterStructDescriptor(compileOptions),
		    .sourceHash = includeHashResult.sourceHash,
		    .includeClosureHash = includeHashResult.includeClosureHash,
		    .optionsHash = optionsHash,
		    .cacheKey = ShaderCacheKey::Compute(
		        package,
		        stage,
		        compileOptions,
		        includeHashResult.sourceHash,
		        includeHashResult.includeClosureHash,
		        optionsHash,
		        backendName,
		        backend->GetBackendVersion())});
	}

	outErrorMessage.clear();
	return true;
}