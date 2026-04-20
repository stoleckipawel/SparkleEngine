#include "PCH.h"

#include "Compiler/ShaderCompileOptionsBuilder.h"

#include "Core/Public/Assets/AssetTypes.h"
#include "Core/Public/FileSystemUtils.h"
#include "Core/Public/Paths/PathUtils.h"

ShaderCompileOptions ShaderCompileOptionsBuilder::Build(const ShaderCookStageDesc& stage)
{
	ShaderCompileOptions options{};
	options.SourcePath = Filesystem::ResolveAssetPathValidated(stage.sourcePath, AssetType::Shader);
	options.EntryPoint = stage.entryPoint;
	options.Stage = stage.stage;

	const std::filesystem::path& projectShaderRoot = Filesystem::GetShaderPath(PathRoot::Project);
	const std::filesystem::path& engineShaderRoot = Filesystem::GetShaderPath(PathRoot::Engine);

	if (!projectShaderRoot.empty())
	{
		options.IncludeDir = projectShaderRoot;
		if (!engineShaderRoot.empty() &&
		    Engine::Paths::MakePathKey(engineShaderRoot) != Engine::Paths::MakePathKey(projectShaderRoot))
		{
			options.AdditionalIncludeDirs.push_back(engineShaderRoot);
		}
	}
	else
	{
		options.IncludeDir = engineShaderRoot;
	}

#if defined(ENGINE_SHADERS_DEBUG)
	options.EnableDebugInfo = true;
	options.StripDebugInfo = false;
#endif

	options.EnableOptimizations = true;

	return options;
}
