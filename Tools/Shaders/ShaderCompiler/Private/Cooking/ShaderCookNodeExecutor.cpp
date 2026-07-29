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

#include "Core/Public/Diagnostics/Error.h"

#include <format>
#include <memory>

CookedStageBuild ShaderCookNodeExecutor::Execute(
    const ShaderPackageCookSettings& settings,
    const CookNode& node,
    const std::filesystem::path& cacheDirectory)
{
	std::unique_ptr<IShaderBackend> backend = CreateShaderBackend(node.backendName);
	LocalDiskShaderArtifactStore artifactStore(cacheDirectory);
	if (std::optional<CookedStageBuild> cachedStage = LoadFromCache(settings, node, artifactStore))
	{
		return std::move(*cachedStage);
	}

	return Compile(settings, node, *backend, artifactStore);
}

std::optional<CookedStageBuild> ShaderCookNodeExecutor::LoadFromCache(
    const ShaderPackageCookSettings& settings,
    const CookNode& node,
    IShaderArtifactStore& artifactStore)
{
	if (!settings.useCache || !settings.debugArtifactDirectory.empty())
	{
		return std::nullopt;
	}

	std::optional<CookedStageBuild> cachedStage = artifactStore.Find(node.cacheKey);
	if (!cachedStage)
	{
		return std::nullopt;
	}

	ApplyNodeMetadata(node, "hit", *cachedStage);
	ShaderParameterStructCookVerifier::Verify(node, *cachedStage, nullptr);
	return cachedStage;
}

CookedStageBuild ShaderCookNodeExecutor::Compile(
    const ShaderPackageCookSettings& settings,
    const CookNode& node,
    IShaderBackend& backend,
    IShaderArtifactStore& artifactStore)
{
	const bool writeDebugArtifacts = !settings.debugArtifactDirectory.empty();
	ShaderDebugArtifactSet debugArtifacts;
	CookedStageBuild compiledStage;
	try
	{
		compiledStage = StageCompiler::Compile(
		    backend,
		    *node.stage,
		    node.compileOptions,
		    writeDebugArtifacts ? &debugArtifacts : nullptr);
	}
	catch (const Diagnostics::Error& error)
	{
		throw Diagnostics::Error(std::format(
		    "Failed to compile {} - {}",
		    ShaderCookDiagnostics::FormatNodeContext(node, backend.GetBackendName(), node.compileOptions.Target),
		    error.what()));
	}

	ShaderParameterStructCookVerifier::Verify(node, compiledStage, &debugArtifacts);
	ApplyNodeMetadata(
	    node,
	    settings.useCache ? (writeDebugArtifacts ? "disabled-debug-artifacts" : "miss") : "disabled",
	    compiledStage);

	PublishArtifacts(settings, node, artifactStore, debugArtifacts, compiledStage);
	return compiledStage;
}

void ShaderCookNodeExecutor::PublishArtifacts(
    const ShaderPackageCookSettings& settings,
    const CookNode& node,
    IShaderArtifactStore& artifactStore,
    const ShaderDebugArtifactSet& debugArtifacts,
    const CookedStageBuild& compiledStage)
{
	const bool writeDebugArtifacts = !settings.debugArtifactDirectory.empty();

	if (writeDebugArtifacts)
	{
		ShaderDebugArtifactWriter::Write(
	        settings.debugArtifactDirectory,
	        *node.package,
	        *node.stage,
	        node.compileOptions,
	        compiledStage,
	        debugArtifacts);
	}

	if (settings.useCache)
	{
		artifactStore.Put(node.cacheKey, compiledStage);
	}
}

void ShaderCookNodeExecutor::ApplyNodeMetadata(const CookNode& node, std::string_view cacheStatus, CookedStageBuild& compiledStage)
{
	if (compiledStage.codegenTarget.empty())
	{
		compiledStage.codegenTarget.assign(GetShaderTargetName(node.compileOptions.Target));
	}

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
