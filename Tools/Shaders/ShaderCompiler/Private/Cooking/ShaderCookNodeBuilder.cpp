#include "PCH.h"

#include "Cooking/ShaderCookNodeBuilder.h"

#include "Backend/IShaderBackend.h"
#include "Backend/ShaderBackendPool.h"
#include "Cooking/Cache/IncludeClosureHasher.h"
#include "Cooking/Cache/ShaderCacheKey.h"
#include "Cooking/Cache/ShaderCompileOptionsHasher.h"
#include "Cooking/ShaderCookPlanner.h"
#include "Cooking/ShaderCookSettings.h"
#include "Compiler/ShaderCompileProfile.h"

#include <format>
#include <utility>

bool ShaderCookNodeBuilder::BuildAndAdd(
    const ShaderPackageCookSettings& settings,
    std::size_t packageIndex,
    std::size_t stageIndex,
    std::size_t targetIndex,
    ShaderBackendPool& backendPool,
    ShaderCookPipelinePlan& plan,
    std::string& outErrorMessage)
{
	const ShaderCookPackageDesc& package = plan.packages[packageIndex];
	const ShaderCookStageDesc& stage = package.stages[stageIndex];
	const ShaderTarget target = settings.targets[targetIndex];
	ShaderCompileOptions compileOptions =
	    BuildCompileOptions(settings, package, stage, target);

	IShaderBackend* backend = nullptr;
	std::string backendName;
	if (!ResolveBackend(
	        settings,
	        package,
	        stage,
	        target,
	        compileOptions,
	        backendPool,
	        backend,
	        backendName,
	        outErrorMessage))
	{
		return false;
	}

	return AppendNode(
	    packageIndex,
	    package,
	    stage,
	    target,
	    std::move(compileOptions),
	    backendName,
	    *backend,
	    plan,
	    outErrorMessage);
}

ShaderCompileOptions ShaderCookNodeBuilder::BuildCompileOptions(
    const ShaderPackageCookSettings& settings,
    const ShaderCookPackageDesc& package,
    const ShaderCookStageDesc& stage,
    ShaderTarget target)
{
	ShaderCompileOptions compileOptions =
	    ShaderCookPlanner::BuildCompileOptions(stage);
	compileOptions.Target = target;
	compileOptions.CaptureDebugArtifacts =
	    !settings.debugArtifactDirectory.empty();
	compileOptions.EnableDebugInfo = settings.enableDebugInfo;
	compileOptions.EnableOptimizations = settings.enableOptimizations;
	compileOptions.TreatWarningsAsErrors = settings.treatWarningsAsErrors;
	compileOptions.StripDebugInfo = settings.stripDebugInfo;

	AppendDescriptorBindingRemaps(package, compileOptions);
	return compileOptions;
}

void ShaderCookNodeBuilder::AppendDescriptorBindingRemaps(
    const ShaderCookPackageDesc& package,
    ShaderCompileOptions& compileOptions)
{
	const std::vector<PassParameterDesc>& parameters = package.bindingLayout.GetParameters();
	compileOptions.DescriptorBindingRemaps.reserve(parameters.size());
	for (std::uint32_t parameterIndex = 0; parameterIndex < parameters.size(); ++parameterIndex)
	{
		const PassParameterDesc& parameter = parameters[parameterIndex];
		if (parameter.Kind == ShaderParameterSemanticKind::RenderTarget ||
		    parameter.Kind == ShaderParameterSemanticKind::DepthTarget)
		{
			continue;
		}
		compileOptions.DescriptorBindingRemaps.push_back(
		    ShaderDescriptorBindingRemap{
		        .Name = std::string(parameter.GetShaderName()),
		        .Set = 0,
		        .Binding = parameterIndex});
	}
}

bool ShaderCookNodeBuilder::ResolveBackend(
    const ShaderPackageCookSettings& settings,
    const ShaderCookPackageDesc& package,
    const ShaderCookStageDesc& stage,
    ShaderTarget target,
    const ShaderCompileOptions& compileOptions,
    ShaderBackendPool& backendPool,
    IShaderBackend*& outBackend,
    std::string& outBackendName,
    std::string& outErrorMessage)
{
	std::string backendError;
	outBackend = backendPool.ResolveAndAcquire(
	    compileOptions.SourcePath,
	    compileOptions.Target,
	    settings.backendName,
	    outBackendName,
	    backendError);
	if (outBackendName.empty())
	{
		outErrorMessage = std::format(
		    "Failed to select shader backend for shader package '{}' stage '{}' target '{}' - {}",
		    package.packageId,
		    GetShaderStagePrefix(stage.stage),
		    GetShaderTargetName(target),
		    backendError);
		return false;
	}

	if (outBackend == nullptr)
	{
		outErrorMessage = std::format(
		    "Failed to construct shader backend '{}' for shader package '{}' stage '{}' target '{}' - {}",
		    outBackendName,
		    package.packageId,
		    GetShaderStagePrefix(stage.stage),
		    GetShaderTargetName(target),
		    backendError);
		return false;
	}

	return true;
}

bool ShaderCookNodeBuilder::AppendNode(
    std::size_t packageIndex,
    const ShaderCookPackageDesc& package,
    const ShaderCookStageDesc& stage,
    ShaderTarget target,
    ShaderCompileOptions compileOptions,
    const std::string& backendName,
    const IShaderBackend& backend,
    ShaderCookPipelinePlan& plan,
    std::string& outErrorMessage)
{
	const IncludeClosureHashResult includeHashResult = IncludeClosureHasher::Compute(compileOptions);
	if (!includeHashResult.Succeeded())
	{
		outErrorMessage = std::format(
		    "Failed to compute include closure for shader package '{}' stage '{}' target '{}' - {}",
		    package.packageId,
		    GetShaderStagePrefix(stage.stage),
		    GetShaderTargetName(target),
		    includeHashResult.errorMessage);
		return false;
	}

	const std::uint64_t optionsHash = ShaderCompileOptionsHasher::Compute(compileOptions);
	const ShaderCacheKey cacheKey = ShaderCacheKey::Compute(
	    package,
	    includeHashResult.sourceHash,
	    includeHashResult.includeClosureHash,
	    optionsHash,
	    backendName,
	    backend.GetBackendVersion());
	const std::string profileName =
	    ShaderCompileProfile::BuildTargetProfile(compileOptions);

	plan.nodes.push_back(CookNode{
	    .packageIndex = packageIndex,
	    .package = &package,
	    .stage = &stage,
	    .backendName = backendName,
	    .compileOptions = std::move(compileOptions),
	    .parameterStructDescriptor = stage.parameterStructDescriptor,
	    .sourceHash = includeHashResult.sourceHash,
	    .includeClosureHash = includeHashResult.includeClosureHash,
	    .optionsHash = optionsHash,
	    .cacheKey = cacheKey,
	    .jobIdentity = ShaderContractJobIdentity{
	        .packageId = package.packageId,
	        .sourcePath = stage.sourcePath,
	        .entryPoint = stage.entryPoint,
	        .stage = stage.stage,
	        .backendName = backendName,
	        .targetName = GetShaderTargetName(target),
	        .profileName = profileName,
	        .sourceHash = includeHashResult.sourceHash,
	        .includeClosureHash = includeHashResult.includeClosureHash,
	        .optionsHash = optionsHash,
	        .jobKey = cacheKey.value}});

	outErrorMessage.clear();
	return true;
}
