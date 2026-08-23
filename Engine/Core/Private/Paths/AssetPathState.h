#pragma once

#include "Core/Public/Assets/AssetTypes.h"

#include <array>
#include <filesystem>

namespace Filesystem::Private
{
	struct AssetPathState final
	{
		static constexpr std::size_t AssetTypeCount = static_cast<std::size_t>(AssetType::Count);

		std::filesystem::path workspacePath;
		std::filesystem::path buildOutputRootPath;
		std::filesystem::path logsRootPath;
		std::filesystem::path cookedAssetRootPath;
		std::filesystem::path cookedShaderRootPath;
		std::filesystem::path cookedShaderPackageRootPath;
		std::filesystem::path cookedShaderRegistryPath;
		std::filesystem::path cookedTextureRootPath;
		std::filesystem::path cookedSceneManifestRootPath;
		std::filesystem::path cookedMeshRootPath;
		std::filesystem::path cookedMaterialRootPath;
		std::filesystem::path cookedSkeletonRootPath;
		std::filesystem::path cookedAnimationRootPath;
		std::filesystem::path sceneAssetRegistryPath;
		std::filesystem::path shaderRecookSignalPath;
		std::filesystem::path projectPath;
		std::filesystem::path projectAssetsPath;
		std::filesystem::path enginePath;
		std::filesystem::path engineAssetsPath;
		std::filesystem::path workingDirectory;
		std::filesystem::path executableDirectory;
		std::filesystem::path shaderSymbolsOutputPath;
		std::filesystem::path emptyPath;
		std::array<std::filesystem::path, AssetTypeCount> projectTypedPaths{};
		std::array<std::filesystem::path, AssetTypeCount> engineTypedPaths{};
		bool packageRuntimeRoot = false;
	};

	AssetPathState& GetAssetPathState();
	void RebuildProjectPaths(AssetPathState& state);
}
