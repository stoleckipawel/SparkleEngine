#include "PCH.h"

#include "Cooking/ShaderCookGraphExecutor.h"

#include "Backend/IShaderBackend.h"
#include "Backend/ShaderBackendPool.h"
#include "Cooking/Cache/IShaderArtifactStore.h"
#include "Cooking/Execution/SerialCookExecutor.h"
#include "Cooking/ShaderDebugArtifactWriter.h"
#include "Cooking/ShaderPackageCooker.h"
#include "Cooking/StageCompiler.h"
#include "ShaderDebugArtifactSet.h"
#include "Verification/ShaderParameterStructVerifier.h"

#include <format>

static std::string FormatNodeDiagnosticContext(
	const CookNode& node,
	std::string_view backendName,
	ShaderTarget target)
{
	return std::format(
	    "shader package '{}' shader '{}' variant '{}' stage '{}' backend '{}' target '{}'",
	    node.package->packageId,
	    node.stage->sourcePath.generic_string(),
	    node.package->variantId,
	    GetShaderStagePrefix(node.stage->stage),
	    backendName,
	    GetShaderTargetName(target));
}

static void ApplyCacheDiagnostics(
	const CookNode& node,
	std::string_view cacheStatus,
	CookedStageBuild& compiledStage)
{
	compiledStage.sourceHash = node.sourceHash;
	compiledStage.includeClosureHash = node.includeClosureHash;
	compiledStage.optionsHash = node.optionsHash;
	compiledStage.cacheKey = node.cacheKey.value;
	compiledStage.cacheStatus.assign(cacheStatus);
}

bool ShaderCookGraphExecutor::Execute(
    const ShaderPackageCookSettings& settings,
    bool writeDebugArtifacts,
    ShaderCookPipelinePlan& plan,
    ShaderBackendPool& backendPool,
    IShaderArtifactStore& artifactStore,
    ShaderCookExecutionCounters& counters,
    std::string& outErrorMessage)
{
	SerialCookExecutor executor;
	return executor.Execute(
	    plan.graph,
	    [&](const CookNode& node, std::string& visitorErrorMessage) -> bool
	    {
		    return ExecuteNode(
		        settings,
		        writeDebugArtifacts,
		        node,
		        plan,
		        backendPool,
		        artifactStore,
		        counters,
		        visitorErrorMessage);
	    },
	    outErrorMessage);
}

bool ShaderCookGraphExecutor::ExecuteNode(
    const ShaderPackageCookSettings& settings,
    bool writeDebugArtifacts,
    const CookNode& node,
    ShaderCookPipelinePlan& plan,
    ShaderBackendPool& backendPool,
    IShaderArtifactStore& artifactStore,
    ShaderCookExecutionCounters& counters,
    std::string& outErrorMessage)
{
	IShaderBackend* backend = backendPool.Find(node.backendName);
	if (backend == nullptr)
	{
		outErrorMessage = "Selected shader backend '" + node.backendName + "' is unavailable";
		return false;
	}

	CookedStageBuild compiledStage;
	if (settings.useCache && !writeDebugArtifacts)
	{
		std::string cacheLookupError;
		if (artifactStore.TryGet(node.cacheKey, compiledStage, cacheLookupError))
		{
			ApplyCacheDiagnostics(node, "hit", compiledStage);
			if (!VerifyParameterStruct(settings, node, compiledStage, nullptr, outErrorMessage))
			{
				return false;
			}

			++counters.cacheHitCount;
			plan.packageContexts[node.packageIndex].compiledStages.push_back(std::move(compiledStage));
			outErrorMessage.clear();
			return true;
		}

		if (!cacheLookupError.empty())
		{
			outErrorMessage = cacheLookupError;
			return false;
		}
	}

	++counters.cacheMissCount;
	++counters.backendInvocationCount;
	ShaderDebugArtifactSet debugArtifacts;
	if (!StageCompiler::Compile(
	        *backend,
	        *node.stage,
	        node.compileOptions,
	        compiledStage,
	        writeDebugArtifacts ? &debugArtifacts : nullptr,
	        outErrorMessage))
	{
		outErrorMessage = std::format(
		    "Failed to compile {} - {}",
		    FormatNodeDiagnosticContext(node, backend->GetBackendName(), node.compileOptions.Target),
		    outErrorMessage);
		return false;
	}

	if (!VerifyParameterStruct(settings, node, compiledStage, &debugArtifacts, outErrorMessage))
	{
		return false;
	}
	ApplyCacheDiagnostics(node, settings.useCache ? (writeDebugArtifacts ? "disabled-debug-artifacts" : "miss") : "disabled", compiledStage);

	if (writeDebugArtifacts &&
	    !ShaderDebugArtifactWriter::Write(
	        settings.debugArtifactDirectory,
	        *node.package,
	        *node.stage,
	        node.compileOptions,
	        compiledStage,
	        debugArtifacts,
	        outErrorMessage))
	{
		outErrorMessage = std::format(
		    "Failed to write debug artifacts for {} - {}",
		    FormatNodeDiagnosticContext(node, compiledStage.backendName, node.compileOptions.Target),
		    outErrorMessage);
		return false;
	}

	if (settings.useCache)
	{
		std::string cachePutError;
		if (!artifactStore.Put(node.cacheKey, compiledStage, cachePutError))
		{
			outErrorMessage = cachePutError;
			return false;
		}
	}

	plan.packageContexts[node.packageIndex].compiledStages.push_back(std::move(compiledStage));
	outErrorMessage.clear();
	return true;
}

bool ShaderCookGraphExecutor::VerifyParameterStruct(
    const ShaderPackageCookSettings& settings,
    const CookNode& node,
    const CookedStageBuild& compiledStage,
    ShaderDebugArtifactSet* debugArtifacts,
    std::string& outErrorMessage)
{
	if (!node.parameterStructDescriptor.has_value())
	{
		return true;
	}
	if (node.package->packageKind == CookedShaderPackageKind::RayTracingLibrary)
	{
		return true;
	}

	ShaderParameterStructDescriptor descriptor = *node.parameterStructDescriptor;
	if (settings.forceParameterStructMismatchForValidation)
	{
		descriptor.Fields.push_back(ShaderParameterStructFieldDescriptor{
		    .Name = "__DeliberateMissingBindingForSelfTest",
		    .LayoutName = "__DeliberateMissingBindingForSelfTest",
		    .ShaderName = "__DeliberateMissingBindingForSelfTest",
		    .Kind = CookedShaderResourceKind::ConstantBuffer,
		    .Dimension = CookedShaderResourceDimension::Buffer,
		    .SemanticKind = ShaderParameterSemanticKind::UniformData,
		    .ResourceDomain = ShaderParameterResourceDomain::Uniform,
		    .Access = ShaderParameterAccess::None,
		    .ArrayCount = 1,
		    .ValueSizeInBytes = sizeof(std::uint32_t),
		    .ValueAlignmentInBytes = alignof(std::uint32_t),
		    .Reflected = true});
	}

	const ShaderParameterStructVerificationResult verificationResult =
	    ShaderParameterStructVerifier::Verify(descriptor, compiledStage.reflection);
	if (debugArtifacts != nullptr)
	{
		debugArtifacts->ParameterMatchReportJson = verificationResult.BuildJsonReport();
	}
	if (!verificationResult.succeeded)
	{
		outErrorMessage = std::format(
		    "SC2001 {} parameter-struct '{}' verification failed: {}",
		    FormatNodeDiagnosticContext(node, compiledStage.backendName, node.compileOptions.Target),
		    descriptor.Name,
		    verificationResult.diagnostics.empty() ? "unknown mismatch" : verificationResult.diagnostics.front());
		return false;
	}

	outErrorMessage.clear();
	return true;
}