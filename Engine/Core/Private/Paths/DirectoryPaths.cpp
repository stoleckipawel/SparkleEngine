#include "PCH.h"

#include "Core/Public/Paths/DirectoryPaths.h"

#include "Core/Public/FileSystemUtils.h"
#include "Core/Public/Formatting/HexFormat.h"
#include "Core/Public/Paths/PathFormatting.h"
#include "Paths/LogPathPolicy.h"

#include <system_error>

namespace Paths
{
	std::array<std::filesystem::path, 2> ExecutableLookupCandidates(std::string_view executableFileName)
	{
		const std::filesystem::path executableDirectory = Filesystem::GetExecutableDirectory();
		const std::filesystem::path executableName{std::string(executableFileName)};
		return {executableDirectory / executableName, executableDirectory.parent_path() / executableName};
	}

	std::filesystem::path LogFile(std::string_view configuredFile, bool ensureParentExists)
	{
		std::filesystem::path configuredPath{std::string(configuredFile)};
		if (!configuredFile.empty() && !configuredPath.empty())
		{
			if (!configuredPath.is_absolute())
			{
				configuredPath = Filesystem::ResolveLogsRootPath() / configuredPath;
			}
			if (ensureParentExists)
			{
				std::error_code errorCode;
				std::filesystem::create_directories(configuredPath.parent_path(), errorCode);
			}
			return configuredPath;
		}

		const std::string executableStem = PathFormatting::SanitizePathSegment(Filesystem::GetExecutablePath().stem().string());
		return Private::DefaultLogDirectory(ensureParentExists, executableStem) /
		       PathFormatting::TimestampedFileName(executableStem, ".log");
	}

	std::filesystem::path CookedShaderPackage(std::uint64_t packageKey)
	{
		return Filesystem::GetCookedShaderPackageRootPath() / (Formatting::FormatHexUInt64(packageKey) + ".sparkshader");
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
		return Filesystem::GetBuildOutputRootPath() / "Cache" / "ImportedTextures";
	}

	std::filesystem::path ShaderRecookSignal(const std::filesystem::path& shaderCacheRoot)
	{
		return Filesystem::BuildShaderRecookSignalPath(shaderCacheRoot);
	}
}  // namespace Paths
