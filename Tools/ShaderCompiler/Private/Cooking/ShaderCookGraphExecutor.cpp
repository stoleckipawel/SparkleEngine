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

#include <iostream>
#include <format>

namespace
{
	std::size_t CountPackageJobs(const ShaderCookPackageDesc& package, const ShaderPackageCookSettings& settings) noexcept
	{
		return package.stages.size() * settings.targets.size();
	}

	void PrintCookPlanSummary(const ShaderCookPipelinePlan& plan, const ShaderPackageCookSettings& settings)
	{
		std::cout << "ShaderCompiler: cooking " << plan.packages.size() << " package(s), "
		          << plan.graph.Size() << " stage job(s), backend='" << settings.backendName
		          << "', cache=" << (settings.useCache ? "enabled" : "disabled") << "\n";
	}

	void PrintPackageProgress(
		const ShaderCookPipelinePlan& plan,
		const ShaderPackageCookSettings& settings,
		const CookNode& node)
	{
		const ShaderCookPackageDesc& package = plan.packages[node.packageIndex];
		std::cout << "ShaderCompiler: package [" << (node.packageIndex + 1) << '/' << plan.packages.size() << "] '"
		          << package.packageId << "' jobs="
		          << CountPackageJobs(package, settings) << "\n";
	}

	void PrintStageProgress(
		const ShaderCookPipelinePlan& plan,
		const ShaderCookExecutionCounters& counters,
		const CookNode& node,
		std::string_view backendName,
		std::string_view status)
	{
		std::cout << "  [" << (counters.processedNodeCount + 1) << '/' << plan.graph.Size() << "] "
		          << status
		          << " package='" << node.package->packageId
		          << "' stage='" << GetShaderStagePrefix(node.stage->stage)
		          << "' target='" << GetShaderTargetName(node.compileOptions.Target)
		          << "' backend='" << backendName << "'"
		          << " source='" << node.stage->sourcePath.generic_string() << "'\n";
	}
}

static std::string FormatNodeDiagnosticContext(
	const CookNode& node,
	std::string_view backendName,
	ShaderTarget target)
{
	return std::format(
	    "shader package '{}' shader '{}' stage '{}' backend '{}' target '{}'",
	    node.package->packageId,
	    node.stage->sourcePath.generic_string(),
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
	PrintCookPlanSummary(plan, settings);
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

	if (plan.packageContexts[node.packageIndex].compiledStages.empty())
	{
		PrintPackageProgress(plan, settings, node);
	}

	CookedStageBuild compiledStage;
	if (settings.useCache && !writeDebugArtifacts)
	{
		std::string cacheLookupError;
		if (artifactStore.TryGet(node.cacheKey, compiledStage, cacheLookupError))
		{
			PrintStageProgress(plan, counters, node, backend->GetBackendName(), "cache-hit");
			ApplyCacheDiagnostics(node, "hit", compiledStage);
			if (!VerifyParameterStruct(settings, node, compiledStage, nullptr, outErrorMessage))
			{
				return false;
			}

			++counters.cacheHitCount;
			++counters.processedNodeCount;
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

	PrintStageProgress(plan, counters, node, backend->GetBackendName(), settings.useCache ? "compiling-cache-miss" : "compiling");
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
	++counters.processedNodeCount;
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