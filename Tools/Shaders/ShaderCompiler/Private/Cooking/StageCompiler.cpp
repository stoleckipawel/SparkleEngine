#include "PCH.h"

#include "Cooking/StageCompiler.h"

#include "Backend/IShaderBackend.h"
#include "Core/Public/Diagnostics/Error.h"
#include "Core/Public/Hash/HashUtils.h"
#include "Core/Public/Paths/PathUtils.h"

CookedStageBuild StageCompiler::Compile(
    IShaderBackend& backend,
    const ShaderCompileRequest& request,
    ShaderDebugArtifactSet* outDebugArtifacts)
{
	const ShaderBackendCapabilities capabilities = backend.GetCapabilities();
	if (!capabilities.SupportsTarget(request.Target))
	{
		throw Diagnostics::Error(
		    std::string{"Active shader backend does not support target '"} + GetShaderTargetName(request.Target) + "'.");
	}
	if (request.UnitKind == ShaderCompileUnitKind::Library && !capabilities.SupportsRayTracingLibrary(request.Target))
	{
		throw Diagnostics::Error(
		    std::string{"Active shader backend does not support shader libraries for target '"} + GetShaderTargetName(request.Target)
		    + "'.");
	}
	if (HasShaderCompileFeature(request.RequiredFeatures, ShaderCompileFeatureFlags::InlineRayQuery)
	    && !capabilities.SupportsInlineRayQuery(request.Target))
	{
		throw Diagnostics::Error(
		    std::string{"Active shader backend does not support inline ray queries for target '"} + GetShaderTargetName(request.Target)
		    + "'.");
	}

	CompiledShader compiledShader = backend.Compile(request);
	const ShaderBytecode bytecode = compiledShader.GetBytecode();
	if (!bytecode.IsValid())
	{
		throw Diagnostics::Error("Backend returned empty bytecode for shader source '" + request.VirtualSourcePath + "'.");
	}

	const auto* bytecodeBegin = static_cast<const std::uint8_t*>(bytecode.Data);
	CookedStageBuild compiledStage;
	compiledStage.stage = request.Stage;
	compiledStage.format = IsSpirVTarget(request.Target) ? ShaderBinaryFormat::SpirV : ShaderBinaryFormat::Dxil;
	compiledStage.sourcePath = request.VirtualSourcePath;
	compiledStage.entryPoint = request.EntryPoint;
	compiledStage.debugArtifact = Paths::MakeProjectRelativeString(compiledShader.GetDebugArtifactPath());
	compiledStage.backendName.assign(backend.GetBackendName());
	compiledStage.codegenTarget.assign(GetShaderTargetName(request.Target));
	compiledStage.backendVersion = backend.GetBackendVersion();
	compiledStage.bytecode.assign(bytecodeBegin, bytecodeBegin + bytecode.Size);
	compiledStage.bytecodeHash = Hash::Fnv1a64(compiledStage.bytecode.data(), compiledStage.bytecode.size());
	compiledStage.reflection = compiledShader.TakeReflection();
	if (outDebugArtifacts != nullptr)
	{
		*outDebugArtifacts = compiledShader.TakeDebugArtifacts();
	}
	return compiledStage;
}
