#include "PCH.h"

#include "Cooking/ShaderCookNodeBuilder.h"

#include "Backend/IShaderBackend.h"
#include "Backend/ShaderBackendPool.h"
#include "Cooking/Cache/IncludeClosureHasher.h"
#include "Cooking/Cache/ShaderCacheKey.h"
#include "Cooking/Cache/ShaderCompileOptionsHasher.h"
#include "Cooking/ShaderCookPlanner.h"
#include "Cooking/ShaderPackageCooker.h"
#include "Compiler/ShaderCompileProfile.h"

#include <format>

bool ShaderCookNodeBuilder::BuildAndAdd(
    const ShaderPackageCookSettings& settings,
    bool writeDebugArtifacts,
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
	ShaderCompileOptions compileOptions = ShaderCookPlanner::BuildCompileOptions(stage);
	compileOptions.Target = target;
	compileOptions.CaptureDebugArtifacts = writeDebugArtifacts;
	compileOptions.EnableDebugInfo = settings.enableDebugInfo;
	compileOptions.EnableOptimizations = settings.enableOptimizations;
	compileOptions.TreatWarningsAsErrors = settings.treatWarningsAsErrors;
	compileOptions.StripReflection = settings.stripReflection;
	compileOptions.StripDebugInfo = settings.stripDebugInfo;

	if (settings.forceMissingIncludeForValidation && packageIndex == 0 && stageIndex == 0 && targetIndex == 0)
	{
		std::string validationError;
		if (!IncludeClosureHasher::ResolveValidationInclude(
		        compileOptions.SourcePath,
		        "__SparkleMissingIncludeSelfTest__.hlsli",
		        compileOptions,
		        validationError))
		{
			outErrorMessage = std::format(
			    "Missing-include verification self-test confirmed unresolved include handling for shader package '{}' stage '{}': {}",
			    package.packageId,
			    GetShaderStagePrefix(stage.stage),
			    validationError);
			return false;
		}
	}

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
		    "Failed to select shader backend for shader package '{}' stage '{}' target '{}' - {}",
		    package.packageId,
		    GetShaderStagePrefix(stage.stage),
		    GetShaderTargetName(target),
		    backendError);
		return false;
	}
	if (backend == nullptr)
	{
		outErrorMessage = std::format(
		    "Failed to construct shader backend '{}' for shader package '{}' stage '{}' target '{}' - {}",
		    backendName,
		    package.packageId,
		    GetShaderStagePrefix(stage.stage),
		    GetShaderTargetName(target),
		    backendError);
		return false;
	}

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
	    stage,
	    compileOptions,
	    includeHashResult.sourceHash,
	    includeHashResult.includeClosureHash,
	    optionsHash,
	    backendName,
	    backend->GetBackendVersion());
	plan.nodes.push_back(CookNode{
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
	    .cacheKey = cacheKey,
	    .jobIdentity = ShaderContractJobIdentity{
	        .packageId = package.packageId,
	        .sourcePath = stage.sourcePath,
	        .entryPoint = stage.entryPoint,
	        .stage = stage.stage,
	        .backendName = backendName,
	        .targetName = GetShaderTargetName(target),
	        .profileName = ShaderCompileProfile::BuildTargetProfile(compileOptions),
	        .sourceHash = includeHashResult.sourceHash,
	        .includeClosureHash = includeHashResult.includeClosureHash,
	        .optionsHash = optionsHash,
	        .jobKey = cacheKey.value}});

	outErrorMessage.clear();
	return true;
}
