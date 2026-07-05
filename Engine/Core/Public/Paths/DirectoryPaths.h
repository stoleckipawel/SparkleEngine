#pragma once

#include "Core/Public/FileSystemUtils.h"
#include "Core/Public/Formatting/HexFormat.h"

#include <array>
#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>

namespace Paths
{
	inline std::array<std::filesystem::path, 3> ExecutableLookupCandidates(std::string_view executableFileName)
	{
		const std::filesystem::path executableDirectory = Filesystem::GetExecutableDirectory();
		const std::filesystem::path executableName{std::string(executableFileName)};
		return {
		    executableDirectory / executableName,
		    executableDirectory.parent_path() / executableName,
		    executableDirectory.parent_path() / "Debug" / executableName};
	}

	// Log files
	std::filesystem::path LogFile(std::string_view configuredFile = {}, bool ensureParentExists = true);

	// Cooked shader files

	inline std::filesystem::path CookedShaderPackage(std::uint64_t packageKey)
	{
		return Filesystem::GetCookedShaderPackageRootPath() / (Formatting::FormatHexUInt64(packageKey) + ".sparkshader");
	}

	// Cooked scene/material/mesh/texture files

	inline std::filesystem::path CookedSceneManifest(std::string_view sceneAssetId)
	{
		std::filesystem::path relativeScenePath{std::string(sceneAssetId)};
		relativeScenePath.replace_extension(".sscn");
		return Filesystem::GetCookedSceneManifestRootPath() / relativeScenePath;
	}

	inline std::filesystem::path CookedSceneManifestRelative(const std::filesystem::path& relativeManifestPath)
	{
		return Filesystem::GetCookedSceneManifestRootPath() / relativeManifestPath;
	}

	inline std::filesystem::path CookedMeshAsset(std::uint64_t meshAssetId)
	{
		return Filesystem::GetCookedMeshRootPath() / (Formatting::FormatHexUInt64(meshAssetId) + ".smsh");
	}

	inline std::filesystem::path CookedMaterialAsset(std::uint64_t materialAssetId)
	{
		return Filesystem::GetCookedMaterialRootPath() / (Formatting::FormatHexUInt64(materialAssetId) + ".smat");
	}

	inline std::filesystem::path CookedSkeletonAsset(std::uint64_t skeletonAssetId)
	{
		return Filesystem::GetCookedSkeletonRootPath() / (Formatting::FormatHexUInt64(skeletonAssetId) + ".sskel");
	}

	inline std::filesystem::path CookedAnimationAsset(std::uint64_t animationAssetId)
	{
		return Filesystem::GetCookedAnimationRootPath() / (Formatting::FormatHexUInt64(animationAssetId) + ".sanim");
	}

	// Shader cache/control files

	inline std::filesystem::path ShaderRecookSignal(const std::filesystem::path& shaderCacheRoot)
	{
		return Filesystem::BuildShaderRecookSignalPath(shaderCacheRoot);
	}
}  // namespace Paths
