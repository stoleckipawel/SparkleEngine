#include "PCH.h"

#include "Cooking/ShaderCookNodeExecutor.h"

#include "Backend/IShaderBackend.h"
#include "Backend/ShaderBackendFactory.h"
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
    const CookNode& node)
{
	std::unique_ptr<IShaderBackend> backend = CreateShaderBackend(node.backendName);
	return Compile(settings, node, *backend);
}

CookedStageBuild ShaderCookNodeExecutor::Compile(
    const ShaderPackageCookSettings& settings,
    const CookNode& node,
    IShaderBackend& backend)
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
	ApplyNodeMetadata(node, compiledStage);

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
	return compiledStage;
}

void ShaderCookNodeExecutor::ApplyNodeMetadata(const CookNode& node, CookedStageBuild& compiledStage)
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
	compiledStage.compileInputHash = node.jobIdentity.compileInputHash;
}
