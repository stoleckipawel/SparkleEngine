#include "PCH.h"

#include "Cooking/StageCompiler.h"

#include "Compiler/DxcShaderCompiler.h"
#include "Compiler/ShaderCompileOptionsBuilder.h"
#include "Core/Public/Hash/HashUtils.h"
#include "Core/Public/Paths/PathUtils.h"
#include "RHI/Public/Shaders/ShaderCompileOptions.h"
#include "RHI/Public/Shaders/ShaderCompileResult.h"

bool StageCompiler::Compile(
	const ShaderCookStageDesc& stage,
	CookedStageBuild& outCompiledStage,
	std::string& outErrorMessage)
{
	using Engine::Paths::MakeProjectRelativeString;

	const ShaderCompileOptions options = ShaderCompileOptionsBuilder::Build(stage);
	const ShaderCompileResult compileResult = DxcShaderCompiler::Compile(options);
	if (!compileResult.IsSuccess())
	{
		outErrorMessage = compileResult.GetErrorMessage();
		return false;
	}

	const ShaderBytecode bytecode = compileResult.GetBytecode();
	if (!bytecode.IsValid())
	{
		outErrorMessage = "DXC returned empty bytecode for shader source '" + stage.sourcePath.generic_string() + "'";
		return false;
	}

	const auto* bytecodeBegin = static_cast<const std::uint8_t*>(bytecode.Data);
	outCompiledStage.stage = stage.stage;
	outCompiledStage.sourcePath = stage.sourcePath.generic_string();
	outCompiledStage.entryPoint = stage.entryPoint;
	outCompiledStage.debugArtifact = MakeProjectRelativeString(compileResult.GetDebugArtifactPath());
	outCompiledStage.bytecode.assign(bytecodeBegin, bytecodeBegin + bytecode.Size);
	outCompiledStage.bytecodeHash = Hash::Fnv1a64(outCompiledStage.bytecode.data(), outCompiledStage.bytecode.size());
	outErrorMessage.clear();
	return true;
}
