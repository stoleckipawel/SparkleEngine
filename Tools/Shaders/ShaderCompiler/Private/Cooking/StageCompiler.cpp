#include "PCH.h"

#include "Cooking/StageCompiler.h"

#include "Backend/IShaderBackend.h"
#include "Core/Public/Diagnostics/Error.h"
#include "Core/Public/Hash/HashUtils.h"
#include "Core/Public/Paths/PathUtils.h"

CookedStageBuild StageCompiler::Compile(
	IShaderBackend& backend,
	const ShaderCookStageDesc& stage,
	const ShaderCompileOptions& options,
	ShaderDebugArtifactSet* outDebugArtifacts)
{
	const ShaderBackendCapabilities capabilities = backend.GetCapabilities();
	if (!capabilities.SupportsTarget(options.Target))
	{
		throw Diagnostics::Error(
		    std::string{"Active shader backend does not support target '"} + GetShaderTargetName(options.Target) + "'.");
	}
	if (options.PackageKind == CookedShaderPackageKind::RayTracingLibrary &&
	    !capabilities.SupportsRayTracingLibrary(options.Target))
	{
		throw Diagnostics::Error(
		    std::string{"Active shader backend does not support ray tracing library packages for target '"} +
		    GetShaderTargetName(options.Target) + "'.");
	}
	if (HasCookedShaderPackageFeature(options.PackageFeatures, CookedShaderPackageFeatureFlags::UsesInlineRayQuery) &&
	    !capabilities.SupportsInlineRayQuery(options.Target))
	{
		throw Diagnostics::Error(
		    std::string{"Active shader backend does not support inline ray queries for target '"} +
		    GetShaderTargetName(options.Target) + "'.");
	}

	CompiledShader compiledShader = backend.Compile(options);
	const ShaderBytecode bytecode = compiledShader.GetBytecode();
	if (!bytecode.IsValid())
	{
		throw Diagnostics::Error(
		    "Backend returned empty bytecode for shader source '" + stage.sourcePath.generic_string() + "'.");
	}

	const auto* bytecodeBegin = static_cast<const std::uint8_t*>(bytecode.Data);
	CookedStageBuild compiledStage;
	compiledStage.stage = stage.stage;
	compiledStage.format = IsSpirVTarget(options.Target)
		? CookedShaderBinaryFormat::SpirV
		: CookedShaderBinaryFormat::Dxil;
	compiledStage.sourcePath = stage.sourcePath.generic_string();
	compiledStage.entryPoint = stage.entryPoint;
	compiledStage.debugArtifact = Paths::MakeProjectRelativeString(compiledShader.GetDebugArtifactPath());
	compiledStage.backendName.assign(backend.GetBackendName());
	compiledStage.codegenTarget.assign(GetShaderTargetName(options.Target));
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
