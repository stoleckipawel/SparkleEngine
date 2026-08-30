#include "PCH.h"

#include "Core/Public/FileSystemUtils.h"

#include "AssetPathState.h"
#include "Core/Public/Paths/PathUtils.h"

namespace Filesystem
{
	const std::filesystem::path& GetWorkingDirectory()
	{
		return Private::GetAssetPathState().workingDirectory;
	}

	const std::filesystem::path& GetWorkspaceRootPath()
	{
		return Private::GetAssetPathState().workspacePath;
	}

	const std::filesystem::path& GetBuildOutputRootPath()
	{
		return Private::GetAssetPathState().buildOutputRootPath;
	}

	const std::filesystem::path& GetLogsRootPath()
	{
		return Private::GetAssetPathState().logsRootPath;
	}

	const std::filesystem::path& GetProjectPath()
	{
		return Private::GetAssetPathState().projectPath;
	}

	const std::filesystem::path& GetProjectAssetsPath()
	{
		return Private::GetAssetPathState().projectAssetsPath;
	}

	const std::filesystem::path& GetEnginePath()
	{
		return Private::GetAssetPathState().enginePath;
	}

	const std::filesystem::path& GetEngineAssetsPath()
	{
		return Private::GetAssetPathState().engineAssetsPath;
	}

	void ConfigureProjectRoot(const std::filesystem::path& projectRoot)
	{
		Private::AssetPathState& state = Private::GetAssetPathState();
		state.projectPath = Paths::Normalize(projectRoot);
		state.projectAssetsPath = state.projectPath.empty() ? std::filesystem::path{} : Paths::Normalize(state.projectPath / "Assets");
		Private::RebuildProjectPaths(state);
	}

	const std::filesystem::path& GetCookedAssetRootPath()
	{
		return Private::GetAssetPathState().cookedAssetRootPath;
	}

	const std::filesystem::path& GetCookedShaderRootPath()
	{
		return Private::GetAssetPathState().cookedShaderRootPath;
	}

	const std::filesystem::path& GetGlobalShaderMapPath()
	{
		return Private::GetAssetPathState().globalShaderMapPath;
	}

	const std::filesystem::path& GetCookedShaderLibraryPath()
	{
		return Private::GetAssetPathState().cookedShaderLibraryPath;
	}

	const std::filesystem::path& GetCookedTextureRootPath()
	{
		return Private::GetAssetPathState().cookedTextureRootPath;
	}

	const std::filesystem::path& GetCookedSceneManifestRootPath()
	{
		return Private::GetAssetPathState().cookedSceneManifestRootPath;
	}

	const std::filesystem::path& GetCookedMeshRootPath()
	{
		return Private::GetAssetPathState().cookedMeshRootPath;
	}

	const std::filesystem::path& GetCookedMaterialRootPath()
	{
		return Private::GetAssetPathState().cookedMaterialRootPath;
	}

	const std::filesystem::path& GetCookedSkeletonRootPath()
	{
		return Private::GetAssetPathState().cookedSkeletonRootPath;
	}

	const std::filesystem::path& GetCookedAnimationRootPath()
	{
		return Private::GetAssetPathState().cookedAnimationRootPath;
	}

	const std::filesystem::path& GetSceneAssetRegistryPath()
	{
		return Private::GetAssetPathState().sceneAssetRegistryPath;
	}

	const std::filesystem::path& GetShaderRecookSignalPath()
	{
		return Private::GetAssetPathState().shaderRecookSignalPath;
	}

	std::filesystem::path BuildShaderRecookSignalPath(const std::filesystem::path& cookedShaderRootPath)
	{
		return Paths::Normalize(cookedShaderRootPath / "recook.signal");
	}

	const std::filesystem::path& GetShaderSymbolsOutputPath()
	{
		return Private::GetAssetPathState().shaderSymbolsOutputPath;
	}
}
