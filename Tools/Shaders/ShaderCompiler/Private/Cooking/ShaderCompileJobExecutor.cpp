#include "PCH.h"

#include "Cooking/ShaderCompileJobExecutor.h"

#include "Backend/IShaderBackend.h"
#include "Backend/ShaderBackendFactory.h"
#include "Cooking/ShaderCookDiagnostics.h"
#include "Cooking/StageCompiler.h"
#include "Core/Public/Diagnostics/Error.h"
#include "ShaderDebugArtifactSet.h"

#include <format>
#include <memory>

ShaderCompileResult ShaderCompileJobExecutor::Execute(const ShaderCompileJob& job)
{
	std::unique_ptr<IShaderBackend> backend = CreateShaderBackend(job.BackendName);
	ShaderDebugArtifactSet debugArtifacts;
	CookedStageBuild output;
	try
	{
		output = StageCompiler::Compile(*backend, job.Request, job.Request.CaptureDebugArtifacts ? &debugArtifacts : nullptr);
	}
	catch (const Diagnostics::Error& error)
	{
		throw Diagnostics::Error(
		    std::format(
		        "Failed to compile {} - {}",
		        ShaderCookDiagnostics::FormatJobContext(job, backend->GetBackendName(), job.Request.Target),
		        error.what()));
	}

	output.sourceHash = job.SourceContentHash;
	output.includeClosureHash = job.DependencyClosureHash;
	output.requestHash = job.RequestHash;
	output.compileInputHash = job.InputHash;

	return ShaderCompileResult{
	    .ShaderType = job.Request.ShaderType,
	    .Target = job.Request.Target,
	    .InputHash = job.InputHash,
	    .Output = std::move(output),
	    .DebugArtifacts = std::move(debugArtifacts)};
}
