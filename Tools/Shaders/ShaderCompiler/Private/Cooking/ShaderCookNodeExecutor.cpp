#include "PCH.h"

#include "Cooking/ShaderCookNodeExecutor.h"

#include "Backend/IShaderBackend.h"
#include "Backend/ShaderBackendFactory.h"
#include "Cooking/Cache/IShaderArtifactStore.h"
#include "Cooking/Cache/LocalDiskShaderArtifactStore.h"
#include "Cooking/ShaderCookDiagnostics.h"
#include "Cooking/ShaderCookSettings.h"
#include "Cooking/ShaderDebugArtifactWriter.h"
#include "Cooking/StageCompiler.h"
#include "ShaderDebugArtifactSet.h"
#include "Verification/ShaderParameterStructCookVerifier.h"

#include <format>
#include <memory>

void ShaderCookNodeExecutor::Execute(
    const ShaderPackageCookSettings& settings,
    const CookNode& node,
    const std::filesystem::path& cacheDirectory,
    ShaderCookNodeResult& outResult)
{
	outResult = {};
	std::unique_ptr<IShaderBackend> backend = CreateShaderBackend(node.backendName, outResult.Diagnostic);
	if (!backend)
	{
		outResult.Diagnostic = "Selected shader backend '" + node.backendName + "' is unavailable: " + outResult.Diagnostic;
		return;
	}
	LocalDiskShaderArtifactStore artifactStore(cacheDirectory);
	if (TryLoadFromCache(settings, node, *backend, artifactStore, outResult))
	{
		outResult.Succeeded = true;
		return;
	}
	if (!outResult.Diagnostic.empty())
		return;
	outResult.Succeeded = Compile(settings, node, *backend, artifactStore, outResult);
}

bool ShaderCookNodeExecutor::TryLoadFromCache(
    const ShaderPackageCookSettings& settings,
    const CookNode& node,
    IShaderBackend&,
    IShaderArtifactStore& artifactStore,
    ShaderCookNodeResult& outResult)
{
	if (!settings.useCache || !settings.debugArtifactDirectory.empty())
		return false;
	std::string cacheError;
	if (!artifactStore.TryGet(node.cacheKey, outResult.CompiledStage, cacheError))
	{
		outResult.Diagnostic = std::move(cacheError);
		return false;
	}
	ApplyNodeMetadata(node, "hit", outResult.CompiledStage);
	if (!ShaderParameterStructCookVerifier::Verify(settings, node, outResult.CompiledStage, nullptr, outResult.Diagnostic))
		return false;
	outResult.CacheHit = true;
	return true;
}

bool ShaderCookNodeExecutor::Compile(
    const ShaderPackageCookSettings& settings,
    const CookNode& node,
    IShaderBackend& backend,
    IShaderArtifactStore& artifactStore,
    ShaderCookNodeResult& outResult)
{
	const bool writeDebugArtifacts = !settings.debugArtifactDirectory.empty();
	ShaderDebugArtifactSet debugArtifacts;
	outResult.BackendInvoked = true;
	if (!StageCompiler::Compile(
	        backend,
	        *node.stage,
	        node.compileOptions,
	        outResult.CompiledStage,
	        writeDebugArtifacts ? &debugArtifacts : nullptr,
	        outResult.Diagnostic))
	{
		outResult.Diagnostic = std::format(
		    "Failed to compile {} - {}",
		    ShaderCookDiagnostics::FormatNodeContext(node, backend.GetBackendName(), node.compileOptions.Target),
		    outResult.Diagnostic);
		return false;
	}
	if (!ShaderParameterStructCookVerifier::Verify(settings, node, outResult.CompiledStage, &debugArtifacts, outResult.Diagnostic))
		return false;
	ApplyNodeMetadata(
	    node,
	    settings.useCache ? (writeDebugArtifacts ? "disabled-debug-artifacts" : "miss") : "disabled",
	    outResult.CompiledStage);
	if (writeDebugArtifacts && !ShaderDebugArtifactWriter::Write(
	                               settings.debugArtifactDirectory,
	                               *node.package,
	                               *node.stage,
	                               node.compileOptions,
	                               outResult.CompiledStage,
	                               debugArtifacts,
	                               outResult.Diagnostic))
		return false;
	if (settings.useCache)
	{
		std::string cacheError;
		if (!artifactStore.Put(node.cacheKey, outResult.CompiledStage, cacheError))
		{
			outResult.Diagnostic = std::move(cacheError);
			return false;
		}
	}
	return true;
}

void ShaderCookNodeExecutor::ApplyNodeMetadata(const CookNode& node, std::string_view cacheStatus, CookedStageBuild& compiledStage)
{
	if (compiledStage.codegenTarget.empty())
		compiledStage.codegenTarget.assign(GetShaderTargetName(node.compileOptions.Target));
	if (compiledStage.shaderBlobId == 0)
	{
		compiledStage.shaderBlobId = BuildShaderBlobId(
		    node.package->packageId,
		    compiledStage.entryPoint,
		    {},
		    compiledStage.backendName,
		    compiledStage.codegenTarget,
		    compiledStage.format);
	}
	compiledStage.sourceHash = node.sourceHash;
	compiledStage.includeClosureHash = node.includeClosureHash;
	compiledStage.optionsHash = node.optionsHash;
	compiledStage.cacheKey = node.cacheKey.value;
	compiledStage.cacheStatus.assign(cacheStatus);
}
