#include "PCH.h"

#include "Cooking/ShaderCookNodeBuilder.h"

#include "Backend/IShaderBackend.h"
#include "Backend/ShaderBackendPool.h"
#include "Cooking/Identity/IncludeClosureHasher.h"
#include "Cooking/Identity/ShaderCompileOptionsHasher.h"
#include "Cooking/ShaderCookPlanner.h"
#include "Cooking/ShaderCookSettings.h"
#include "Compiler/ShaderCompileProfile.h"

#include "Core/Public/Diagnostics/Error.h"
#include "Core/Public/Hash/HashUtils.h"

#include <format>
#include <utility>

std::uint64_t ShaderCookNodeBuilder::BuildCompileInputHash(
    const std::uint64_t sourceHash,
    const std::uint64_t includeClosureHash,
    const std::uint64_t optionsHash,
    const std::string_view backendName,
    const std::uint64_t backendVersion)
{
	std::string canonical;
	canonical.reserve(128);
	canonical += std::to_string(sourceHash);
	canonical += '|';
	canonical += std::to_string(includeClosureHash);
	canonical += '|';
	canonical += std::to_string(optionsHash);
	canonical += '|';
	canonical += backendName;
	canonical += '|';
	canonical += std::to_string(backendVersion);
	const std::uint64_t hash = Hash::Fnv1a64(canonical);
	return hash != 0 ? hash : Hash::kFnv64OffsetBasis;
}

void ShaderCookNodeBuilder::BuildAndAdd(
    const ShaderPackageCookSettings& settings,
    std::size_t packageIndex,
    std::size_t stageIndex,
    std::size_t targetIndex,
    ShaderBackendPool& backendPool,
    ShaderCookPipelinePlan& plan)
{
	const ShaderCookPackageDesc& package = plan.packages[packageIndex];
	const ShaderCookStageDesc& stage = package.stages[stageIndex];
	const ShaderTarget target = settings.targets[targetIndex];
	ShaderCompileOptions compileOptions = BuildCompileOptions(settings, package, stage, target);

	IShaderBackend& backend = ResolveBackend(settings, package, stage, target, compileOptions, backendPool);
	const std::string backendName(backend.GetBackendName());

	AppendNode(packageIndex, package, stage, target, std::move(compileOptions), backendName, backend, plan);
}

ShaderCompileOptions ShaderCookNodeBuilder::BuildCompileOptions(
    const ShaderPackageCookSettings& settings,
    const ShaderCookPackageDesc& package,
    const ShaderCookStageDesc& stage,
    ShaderTarget target)
{
	ShaderCompileOptions compileOptions = ShaderCookPlanner::BuildCompileOptions(stage);
	compileOptions.Target = target;
	compileOptions.CaptureDebugArtifacts = !settings.debugArtifactDirectory.empty();
	compileOptions.EnableDebugInfo = settings.enableDebugInfo;
	compileOptions.EnableOptimizations = settings.enableOptimizations;
	compileOptions.TreatWarningsAsErrors = settings.treatWarningsAsErrors;
	compileOptions.StripDebugInfo = settings.stripDebugInfo;

	AppendDescriptorBindingRemaps(package, compileOptions);
	return compileOptions;
}

void ShaderCookNodeBuilder::AppendDescriptorBindingRemaps(const ShaderCookPackageDesc& package, ShaderCompileOptions& compileOptions)
{
	const std::vector<PassParameterDesc>& parameters = package.bindingLayout.GetParameters();
	compileOptions.DescriptorBindingRemaps.reserve(parameters.size());
	for (std::uint32_t parameterIndex = 0; parameterIndex < parameters.size(); ++parameterIndex)
	{
		const PassParameterDesc& parameter = parameters[parameterIndex];
		if (parameter.Kind == ShaderParameterSemanticKind::RenderTarget || parameter.Kind == ShaderParameterSemanticKind::DepthTarget)
		{
			continue;
		}
		compileOptions.DescriptorBindingRemaps.push_back(
		    ShaderDescriptorBindingRemap{.Name = parameter.Name, .Set = 0, .Binding = parameterIndex});
	}
}

IShaderBackend& ShaderCookNodeBuilder::ResolveBackend(
    const ShaderPackageCookSettings& settings,
    const ShaderCookPackageDesc& package,
    const ShaderCookStageDesc& stage,
    ShaderTarget target,
    const ShaderCompileOptions& compileOptions,
    ShaderBackendPool& backendPool)
{
	try
	{
		return backendPool.ResolveAndAcquire(compileOptions.SourcePath, compileOptions.Target, settings.backendName);
	}
	catch (const Diagnostics::Error& error)
	{
		throw Diagnostics::Error(
		    std::format(
		        "Failed to resolve a shader backend for shader package '{}' stage '{}' target '{}' - {}",
		        package.packageId,
		        GetShaderStagePrefix(stage.stage),
		        GetShaderTargetName(target),
		        error.what()));
	}
}

void ShaderCookNodeBuilder::AppendNode(
    std::size_t packageIndex,
    const ShaderCookPackageDesc& package,
    const ShaderCookStageDesc& stage,
    ShaderTarget target,
    ShaderCompileOptions compileOptions,
    const std::string& backendName,
    const IShaderBackend& backend,
    ShaderCookPipelinePlan& plan)
{
	const IncludeClosureHash includeHash = IncludeClosureHasher::Compute(compileOptions);

	const std::uint64_t optionsHash = ShaderCompileOptionsHasher::Compute(compileOptions);
	const std::uint64_t compileInputHash = BuildCompileInputHash(
	    includeHash.sourceHash,
	    includeHash.includeClosureHash,
	    optionsHash,
	    backendName,
	    backend.GetBackendVersion());
	const std::string profileName = ShaderCompileProfile::BuildTargetProfile(compileOptions);

	plan.nodes.push_back(
	    CookNode{
	        .packageIndex = packageIndex,
	        .package = &package,
	        .stage = &stage,
	        .backendName = backendName,
	        .compileOptions = std::move(compileOptions),
	        .parameterStructDescriptor = stage.parameterStructDescriptor,
	        .sourceHash = includeHash.sourceHash,
	        .includeClosureHash = includeHash.includeClosureHash,
	        .optionsHash = optionsHash,
	        .jobIdentity = ShaderContractJobIdentity{
	            .packageId = package.packageId,
	            .sourcePath = stage.sourcePath,
	            .entryPoint = stage.entryPoint,
	            .stage = stage.stage,
	            .backendName = backendName,
	            .targetName = GetShaderTargetName(target),
	            .profileName = profileName,
	            .sourceHash = includeHash.sourceHash,
	            .includeClosureHash = includeHash.includeClosureHash,
	            .optionsHash = optionsHash,
	            .compileInputHash = compileInputHash}});
}
