#include "PCH.h"

#include "Cooking/ShaderCookNodeExecutor.h"

#include "Backend/IShaderBackend.h"
#include "Backend/ShaderBackendPool.h"
#include "Cooking/Cache/IShaderArtifactStore.h"
#include "Cooking/CookedStageBuildFinalizer.h"
#include "Cooking/ShaderCookDiagnostics.h"
#include "Cooking/ShaderCookProgressReporter.h"
#include "Cooking/ShaderDebugArtifactWriter.h"
#include "Cooking/ShaderPackageCooker.h"
#include "Cooking/StageCompiler.h"
#include "ShaderDebugArtifactSet.h"
#include "Verification/ShaderParameterStructCookVerifier.h"

#include <format>
#include <utility>

bool ShaderCookNodeExecutor::Execute(
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
		ShaderCookProgressReporter::PrintPackageProgress(plan, settings, node);
	}

	bool completedFromCache = false;
	if (!TryCompleteFromCache(
	        settings,
	        writeDebugArtifacts,
	        node,
	        *backend,
	        plan,
	        artifactStore,
	        counters,
	        completedFromCache,
	        outErrorMessage))
	{
		return false;
	}
	if (completedFromCache)
	{
		outErrorMessage.clear();
		return true;
	}

	ShaderCookProgressReporter::PrintStageProgress(
	    plan,
	    counters,
	    node,
	    backend->GetBackendName(),
	    settings.useCache ? "compiling-cache-miss" : "compiling");
	++counters.cacheMissCount;
	++counters.backendInvocationCount;

	CookedStageBuild compiledStage;
	ShaderDebugArtifactSet debugArtifacts;
	if (!CompileStage(*backend, node, writeDebugArtifacts, compiledStage, debugArtifacts, outErrorMessage))
	{
		return false;
	}

	if (!ShaderParameterStructCookVerifier::Verify(settings, node, compiledStage, &debugArtifacts, outErrorMessage))
	{
		return false;
	}
	CookedStageBuildFinalizer::ApplyNodeMetadata(
	    node,
	    settings.useCache ? (writeDebugArtifacts ? "disabled-debug-artifacts" : "miss") : "disabled",
	    compiledStage);

	if (!WriteDebugArtifacts(settings, node, compiledStage, debugArtifacts, outErrorMessage))
	{
		return false;
	}

	if (!StoreCompiledStage(settings, node.cacheKey, compiledStage, artifactStore, outErrorMessage))
	{
		return false;
	}

	RecordCompletedStage(node, std::move(compiledStage), plan, counters);
	outErrorMessage.clear();
	return true;
}

bool ShaderCookNodeExecutor::TryCompleteFromCache(
    const ShaderPackageCookSettings& settings,
    bool writeDebugArtifacts,
    const CookNode& node,
    IShaderBackend& backend,
    ShaderCookPipelinePlan& plan,
    IShaderArtifactStore& artifactStore,
    ShaderCookExecutionCounters& counters,
    bool& outCompleted,
    std::string& outErrorMessage)
{
	outCompleted = false;
	if (settings.useCache && !writeDebugArtifacts)
	{
		CookedStageBuild compiledStage;
		std::string cacheLookupError;
		if (artifactStore.TryGet(node.cacheKey, compiledStage, cacheLookupError))
		{
			ShaderCookProgressReporter::PrintStageProgress(plan, counters, node, backend.GetBackendName(), "cache-hit");
			CookedStageBuildFinalizer::ApplyNodeMetadata(node, "hit", compiledStage);
			if (!ShaderParameterStructCookVerifier::Verify(settings, node, compiledStage, nullptr, outErrorMessage))
			{
				return false;
			}

			++counters.cacheHitCount;
			RecordCompletedStage(node, std::move(compiledStage), plan, counters);
			outCompleted = true;
			outErrorMessage.clear();
			return true;
		}

		if (!cacheLookupError.empty())
		{
			outErrorMessage = cacheLookupError;
			return false;
		}
	}

	outErrorMessage.clear();
	return true;
}

bool ShaderCookNodeExecutor::CompileStage(
    IShaderBackend& backend,
    const CookNode& node,
    bool writeDebugArtifacts,
    CookedStageBuild& compiledStage,
    ShaderDebugArtifactSet& debugArtifacts,
    std::string& outErrorMessage)
{
	if (!StageCompiler::Compile(
	        backend,
	        *node.stage,
	        node.compileOptions,
	        compiledStage,
	        writeDebugArtifacts ? &debugArtifacts : nullptr,
	        outErrorMessage))
	{
		outErrorMessage = std::format(
		    "Failed to compile {} - {}",
		    ShaderCookDiagnostics::FormatNodeContext(node, backend.GetBackendName(), node.compileOptions.Target),
		    outErrorMessage);
		return false;
	}

	outErrorMessage.clear();
	return true;
}

bool ShaderCookNodeExecutor::WriteDebugArtifacts(
    const ShaderPackageCookSettings& settings,
    const CookNode& node,
    const CookedStageBuild& compiledStage,
    const ShaderDebugArtifactSet& debugArtifacts,
    std::string& outErrorMessage)
{
	const bool writeDebugArtifacts = !settings.debugArtifactDirectory.empty();

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
		    ShaderCookDiagnostics::FormatNodeContext(node, compiledStage.backendName, node.compileOptions.Target),
		    outErrorMessage);
		return false;
	}

	outErrorMessage.clear();
	return true;
}

bool ShaderCookNodeExecutor::StoreCompiledStage(
    const ShaderPackageCookSettings& settings,
	const ShaderCacheKey& cacheKey,
    const CookedStageBuild& compiledStage,
    IShaderArtifactStore& artifactStore,
    std::string& outErrorMessage)
{
	if (settings.useCache)
	{
		std::string cachePutError;
		if (!artifactStore.Put(cacheKey, compiledStage, cachePutError))
		{
			outErrorMessage = cachePutError;
			return false;
		}
	}

	outErrorMessage.clear();
	return true;
}

void ShaderCookNodeExecutor::RecordCompletedStage(
    const CookNode& node,
    CookedStageBuild&& compiledStage,
    ShaderCookPipelinePlan& plan,
    ShaderCookExecutionCounters& counters)
{
	plan.packageContexts[node.packageIndex].compiledStages.push_back(std::move(compiledStage));
	++counters.processedNodeCount;
}