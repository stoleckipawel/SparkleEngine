#include "PCH.h"

#include "Core/Public/Paths/DirectoryPaths.h"

#include "Core/Public/FileSystemUtils.h"
#include "Core/Public/Formatting/HexFormat.h"
#include "Core/Public/Paths/PathFormatting.h"
#include "Core/Public/Paths/PathUtils.h"
#include "Core/Public/Paths/ProductUserStatePaths.h"
#include "Paths/LogPathPolicy.h"

#include <system_error>

namespace Paths
{
	std::filesystem::path LogFile(std::string_view configuredFile, bool ensureParentExists)
	{
		std::filesystem::path configuredPath{std::string(configuredFile)};
		if (!configuredFile.empty() && !configuredPath.empty())
		{
			const Filesystem::ProductUserStatePaths& userState = Filesystem::GetProductUserStatePaths();
			const std::filesystem::path& logsRoot = userState.LogsRoot;
			if (!configuredPath.is_absolute())
			{
				configuredPath = Paths::Normalize(logsRoot / configuredPath);
			}
			if (Paths::IsUnderRoot(configuredPath, logsRoot))
			{
				if (ensureParentExists)
				{
					std::error_code errorCode;
					std::filesystem::create_directories(configuredPath.parent_path(), errorCode);
				}
				return configuredPath;
			}
		}

		const std::string executableStem = PathFormatting::SanitizePathSegment(Filesystem::GetExecutablePath().stem().string());
		return Private::DefaultLogDirectory(ensureParentExists, executableStem)
		    / PathFormatting::TimestampedFileName(executableStem, ".log");
	}

	std::filesystem::path CookedSceneManifest(std::string_view sceneAssetId)
	{
		std::filesystem::path relativeScenePath{std::string(sceneAssetId)};
		relativeScenePath.replace_extension(".sscn");
		return Filesystem::GetCookedSceneManifestRootPath() / relativeScenePath;
	}

	std::filesystem::path CookedSceneManifestRelative(const std::filesystem::path& relativeManifestPath)
	{
		return Filesystem::GetCookedSceneManifestRootPath() / relativeManifestPath;
	}

	std::filesystem::path CookedMeshAsset(std::uint64_t meshAssetId)
	{
		return Filesystem::GetCookedMeshRootPath() / (Formatting::FormatHexUInt64(meshAssetId) + ".smsh");
	}

	std::filesystem::path CookedMaterialAsset(std::uint64_t materialAssetId)
	{
		return Filesystem::GetCookedMaterialRootPath() / (Formatting::FormatHexUInt64(materialAssetId) + ".smat");
	}

	std::filesystem::path CookedSkeletonAsset(std::uint64_t skeletonAssetId)
	{
		return Filesystem::GetCookedSkeletonRootPath() / (Formatting::FormatHexUInt64(skeletonAssetId) + ".sskel");
	}

	std::filesystem::path CookedAnimationAsset(std::uint64_t animationAssetId)
	{
		return Filesystem::GetCookedAnimationRootPath() / (Formatting::FormatHexUInt64(animationAssetId) + ".sanim");
	}

	std::filesystem::path ImportedTextureCacheRoot()
	{
		const Filesystem::ProductUserStatePaths& userState = Filesystem::GetProductUserStatePaths();
		return userState.CacheRoot / "ImportedTextures";
	}

	std::filesystem::path ShaderRecookSignal(const std::filesystem::path& cookedShaderRoot)
	{
		return Filesystem::BuildShaderRecookSignalPath(cookedShaderRoot);
	}
}
