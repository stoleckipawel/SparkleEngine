#pragma once

#include "Core/Public/CoreAPI.h"

#include <array>
#include <cstdint>
#include <filesystem>
#include <string_view>

namespace Paths
{
	SPARKLE_CORE_API std::array<std::filesystem::path, 2> ExecutableLookupCandidates(std::string_view executableFileName);
	SPARKLE_CORE_API std::filesystem::path LogFile(std::string_view configuredFile = {}, bool ensureParentExists = true);
	SPARKLE_CORE_API std::filesystem::path CookedShaderPackage(std::uint64_t packageKey);
	SPARKLE_CORE_API std::filesystem::path CookedSceneManifest(std::string_view sceneAssetId);
	SPARKLE_CORE_API std::filesystem::path CookedSceneManifestRelative(const std::filesystem::path& relativeManifestPath);
	SPARKLE_CORE_API std::filesystem::path CookedMeshAsset(std::uint64_t meshAssetId);
	SPARKLE_CORE_API std::filesystem::path CookedMaterialAsset(std::uint64_t materialAssetId);
	SPARKLE_CORE_API std::filesystem::path CookedSkeletonAsset(std::uint64_t skeletonAssetId);
	SPARKLE_CORE_API std::filesystem::path CookedAnimationAsset(std::uint64_t animationAssetId);
	SPARKLE_CORE_API std::filesystem::path ImportedTextureCacheRoot();
	SPARKLE_CORE_API std::filesystem::path ShaderRecookSignal(const std::filesystem::path& cookedShaderRoot);
}  // namespace Paths
