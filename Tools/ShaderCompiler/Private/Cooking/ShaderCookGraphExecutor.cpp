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
		    "Failed to compile shader package '{}' variant '{}' stage '{}' - {}",
		    node.package->packageId,
		    node.package->variantId,
		    GetShaderStagePrefix(node.stage->stage),
		    outErrorMessage);
		return false;
	}

	if (!VerifyParameterStruct(settings, node, compiledStage, &debugArtifacts, outErrorMessage))
	{
		return false;
	}

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
		    "Failed to write debug artifacts for shader package '{}' variant '{}' stage '{}' - {}",
		    node.package->packageId,
		    node.package->variantId,
		    GetShaderStagePrefix(node.stage->stage),
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
		    "SC2001 shader package '{}' variant '{}' stage '{}' parameter-struct verification failed: {}",
		    node.package->packageId,
		    node.package->variantId,
		    GetShaderStagePrefix(node.stage->stage),
		    verificationResult.diagnostics.empty() ? "unknown mismatch" : verificationResult.diagnostics.front());
		return false;
	}

	outErrorMessage.clear();
	return true;
}