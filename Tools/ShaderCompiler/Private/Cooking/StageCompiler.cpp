#include "PCH.h"

#include "Cooking/StageCompiler.h"

#include "Backend/IShaderBackend.h"
#include "Core/Public/Hash/HashUtils.h"
#include "Core/Public/Paths/PathUtils.h"
#include "ShaderCompileResult.h"

bool StageCompiler::Compile(
	IShaderBackend& backend,
	const ShaderCookStageDesc& stage,
	const ShaderCompileOptions& options,
	CookedStageBuild& outCompiledStage,
	std::string& outErrorMessage)
{
	using Engine::Paths::MakeProjectRelativeString;

	if (!backend.GetCapabilities().SupportsTarget(options.Target))
	{
		outErrorMessage = std::string{"Active shader backend does not support target '"} +
			GetShaderTargetName(options.Target) + "'";
		return false;
	}

	ShaderCompileResult compileResult = backend.Compile(options);
	if (!compileResult.IsSuccess())
	{
		outErrorMessage = compileResult.GetErrorMessage();
		return false;
	}

	const ShaderBytecode bytecode = compileResult.GetBytecode();
	if (!bytecode.IsValid())
	{
		outErrorMessage = "Backend returned empty bytecode for shader source '" + stage.sourcePath.generic_string() + "'";
		return false;
	}

	const auto* bytecodeBegin = static_cast<const std::uint8_t*>(bytecode.Data);
	outCompiledStage.stage = stage.stage;
	outCompiledStage.format = IsSpirVTarget(options.Target)
		? CookedShaderBinaryFormat::SpirV
		: CookedShaderBinaryFormat::Dxil;
	outCompiledStage.sourcePath = stage.sourcePath.generic_string();
	outCompiledStage.entryPoint = stage.entryPoint;
	outCompiledStage.debugArtifact = MakeProjectRelativeString(compileResult.GetDebugArtifactPath());
	outCompiledStage.backendName.assign(backend.GetBackendName());
	outCompiledStage.backendVersion = backend.GetBackendVersion();
	outCompiledStage.bytecode.assign(bytecodeBegin, bytecodeBegin + bytecode.Size);
	outCompiledStage.bytecodeHash = Hash::Fnv1a64(outCompiledStage.bytecode.data(), outCompiledStage.bytecode.size());
	outCompiledStage.reflection = compileResult.TakeReflection();
	outErrorMessage.clear();
	return true;
}
