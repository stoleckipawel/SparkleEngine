#include "PCH.h"

#include "Cooking/ShaderCompileJobBuilder.h"

#include "Backend/IShaderBackend.h"
#include "Backend/ShaderBackendPool.h"
#include "Compiler/ShaderCompileProfile.h"
#include "Compiler/ShaderSourcePreprocessor.h"
#include "Compiler/ShaderSourceMountTable.h"
#include "Cooking/Identity/IncludeClosureHasher.h"
#include "Cooking/Identity/ShaderCompileRequestHasher.h"
#include "Cooking/ShaderCookSettings.h"
#include "Core/Public/Diagnostics/Error.h"
#include "Core/Public/FileSystemUtils.h"
#include "Core/Public/Hash/HashUtils.h"

#include <format>
#include <utility>

ShaderCompileInputHash ShaderCompileJobBuilder::BuildInputHash(
    std::uint64_t sourceContentHash,
    std::uint64_t dependencyClosureHash,
    std::uint64_t requestHash,
    std::string_view backendName,
    std::uint64_t backendVersion)
{
	std::string canonical;
	canonical.reserve(128);
	canonical += "Sparkle.ShaderCompileInput;";
	canonical += std::to_string(sourceContentHash);
	canonical += '|';
	canonical += std::to_string(dependencyClosureHash);
	canonical += '|';
	canonical += std::to_string(requestHash);
	canonical += '|';
	canonical += std::to_string(backendName.size());
	canonical += ':';
	canonical += backendName;
	canonical += ';';
	canonical += std::to_string(backendVersion);
	const ShaderCompileInputHash hash = Hash::Fnv1a64(canonical);
	return hash != 0 ? hash : Hash::kFnv64OffsetBasis;
}

void ShaderCompileJobBuilder::BuildAndAdd(
    const ShaderPackageCookSettings& settings,
    std::size_t packageIndex,
    std::size_t stageIndex,
    ShaderTarget target,
    ShaderBackendPool& backendPool,
    ShaderCookPipelinePlan& plan)
{
	const ShaderCookPackageDesc& package = plan.packages[packageIndex];
	const ShaderCookStageDesc& stage = package.stages[stageIndex];
	ShaderCompileRequest request = BuildRequest(settings, package, stage, target);

	IShaderBackend* backend = nullptr;
	try
	{
		backend = &backendPool.ResolveAndAcquire(request.VirtualSourcePath, request.Target, settings.backendName);
	}
	catch (const Diagnostics::Error& error)
	{
		throw Diagnostics::Error(
		    std::format(
		        "Failed to resolve a shader backend for shader type '{}' source '{}' target '{}' - {}",
		        request.ShaderTypeName,
		        request.VirtualSourcePath,
		        GetShaderTargetName(request.Target),
		        error.what()));
	}

	const IncludeClosureHash closureBeforeSnapshot = IncludeClosureHasher::Compute(request);
	request.SourceCode = ShaderSourcePreprocessor::Load(request.VirtualSourcePath, request);
	const IncludeClosureHash closure = IncludeClosureHasher::Compute(request);
	if (closureBeforeSnapshot.sourceHash != closure.sourceHash || closureBeforeSnapshot.includeClosureHash != closure.includeClosureHash
	    || closureBeforeSnapshot.virtualDependencies != closure.virtualDependencies)
	{
		throw Diagnostics::Error(
		    std::format(
		        "Shader source closure changed while constructing compile job for '{}' source '{}'; retry the cook.",
		        request.ShaderTypeName,
		        request.VirtualSourcePath));
	}
	const std::uint64_t sourceContentHash = Hash::Fnv1a64(request.SourceCode);
	const std::uint64_t stableSourceContentHash = sourceContentHash != 0 ? sourceContentHash : Hash::kFnv64OffsetBasis;
	const std::uint64_t requestHash = ShaderCompileRequestHasher::Compute(request);
	const std::string backendName(backend->GetBackendName());
	const std::uint64_t backendVersion = backend->GetBackendVersion();
	const std::string targetProfile = ShaderCompileProfile::BuildTargetProfile(request);
	const std::size_t jobIndex = plan.jobs.size();
	plan.jobs.push_back(
	    ShaderCompileJob{
	        .Request = std::move(request),
	        .BackendName = backendName,
	        .TargetProfile = targetProfile,
	        .BackendVersion = backendVersion,
	        .SourceContentHash = stableSourceContentHash,
	        .DependencyClosureHash = closure.includeClosureHash,
	        .RequestHash = requestHash,
	        .InputHash = BuildInputHash(stableSourceContentHash, closure.includeClosureHash, requestHash, backendName, backendVersion),
	        .VirtualDependencies = closure.virtualDependencies});
	plan.consumers.push_back(ShaderCompileConsumer{.JobIndex = jobIndex, .PackageIndex = packageIndex});
}

ShaderCompileRequest ShaderCompileJobBuilder::BuildRequest(
    const ShaderPackageCookSettings& settings,
    const ShaderCookPackageDesc& package,
    const ShaderCookStageDesc& stage,
    ShaderTarget target)
{
	if (stage.packageKind == CookedShaderPackageKind::RayTracingLibrary)
	{
		throw Diagnostics::Error(
		    "Ray-tracing library compile jobs remain disabled until Phase 6 publishes their complete runtime consumer.");
	}

	ShaderCompileRequest request(GetSourceMounts());
	request.ShaderType = stage.shaderTypeId;
	request.ShaderTypeName = stage.shaderTypeName;
	request.VirtualSourcePath = request.SourceMounts.get().CanonicalizeVirtualPath(stage.sourcePath);
	request.EntryPoint = stage.entryPoint;
	request.Stage = stage.stage;
	request.Target = target;
	request.UnitKind = ShaderCompileUnitKind::EntryPoint;
	request.RequiredFeatures = BuildRequiredFeatures(stage.packageFeatures);
	request.ParameterStruct = stage.parameterStructDescriptor;
	request.CaptureDebugArtifacts = !settings.debugArtifactDirectory.empty();
	request.EnableDebugInfo = settings.enableDebugInfo;
	request.EnableOptimizations = settings.enableOptimizations;
	request.TreatWarningsAsErrors = settings.treatWarningsAsErrors;
	request.StripDebugInfo = settings.stripDebugInfo;
	AppendDescriptorBindingRemaps(package, request);
	return request;
}

const ShaderSourceMountTable& ShaderCompileJobBuilder::GetSourceMounts()
{
	static const ShaderSourceMountTable sourceMounts(
	    Filesystem::GetShaderPath(PathRoot::Engine),
	    Filesystem::GetShaderPath(PathRoot::Project));
	return sourceMounts;
}

void ShaderCompileJobBuilder::AppendDescriptorBindingRemaps(const ShaderCookPackageDesc& package, ShaderCompileRequest& request)
{
	const std::vector<PassParameterDesc>& parameters = package.bindingLayout.GetParameters();
	request.DescriptorBindingRemaps.reserve(parameters.size());
	for (std::uint32_t parameterIndex = 0; parameterIndex < parameters.size(); ++parameterIndex)
	{
		const PassParameterDesc& parameter = parameters[parameterIndex];
		if (parameter.Kind == ShaderParameterSemanticKind::RenderTarget || parameter.Kind == ShaderParameterSemanticKind::DepthTarget)
		{
			continue;
		}
		request.DescriptorBindingRemaps.push_back(
		    ShaderDescriptorBindingRemap{.Name = parameter.Name, .Set = 0, .Binding = parameterIndex});
	}
}

ShaderCompileFeatureFlags ShaderCompileJobBuilder::BuildRequiredFeatures(CookedShaderPackageFeatureFlags packageFeatures) noexcept
{
	ShaderCompileFeatureFlags features = ShaderCompileFeatureFlags::None;
	if (HasCookedShaderPackageFeature(packageFeatures, CookedShaderPackageFeatureFlags::UsesInlineRayQuery))
	{
		features |= ShaderCompileFeatureFlags::InlineRayQuery;
	}
	if (HasCookedShaderPackageFeature(packageFeatures, CookedShaderPackageFeatureFlags::UsesAccelerationStructure))
	{
		features |= ShaderCompileFeatureFlags::AccelerationStructure;
	}
	if (HasCookedShaderPackageFeature(packageFeatures, CookedShaderPackageFeatureFlags::UsesDescriptorIndexing))
	{
		features |= ShaderCompileFeatureFlags::DescriptorIndexing;
	}
	return features;
}
