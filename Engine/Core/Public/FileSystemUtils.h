#pragma once

#include "Assets/AssetTypes.h"
#include "Paths/PathRoot.h"

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

namespace Filesystem
{
	inline constexpr std::string_view kWorkspaceMarker = ".sparkle";
	inline constexpr std::string_view kEngineMarker = ".sparkle-engine";
	inline constexpr std::string_view kProjectMarker = ".sparkle-project";

	// Process paths
	const std::filesystem::path& GetWorkingDirectory();
	std::filesystem::path GetExecutablePath();
	std::filesystem::path GetExecutableDirectory();

	// Stateless root discovery for early bootstrap code
	std::filesystem::path ResolveWorkspaceRootPath();
	std::filesystem::path ResolveBuildOutputRootPath();
	std::filesystem::path ResolveLogsRootPath();

	// Cached repository roots
	const std::filesystem::path& GetWorkspaceRootPath();
	const std::filesystem::path& GetBuildOutputRootPath();
	const std::filesystem::path& GetLogsRootPath();
	const std::filesystem::path& GetProjectPath();
	const std::filesystem::path& GetProjectAssetsPath();
	const std::filesystem::path& GetEnginePath();
	const std::filesystem::path& GetEngineAssetsPath();
	void ConfigureProjectRoot(const std::filesystem::path& projectRoot);

	// Generated cooked/cache/output roots
	const std::filesystem::path& GetCookedAssetRootPath();
	const std::filesystem::path& GetCookedShaderRootPath();
	const std::filesystem::path& GetCookedShaderPackageRootPath();
	const std::filesystem::path& GetCookedShaderRegistryPath();
	const std::filesystem::path& GetCookedTextureRootPath();
	const std::filesystem::path& GetCookedSceneManifestRootPath();
	const std::filesystem::path& GetCookedMeshRootPath();
	const std::filesystem::path& GetCookedMaterialRootPath();
	const std::filesystem::path& GetSceneAssetRegistryPath();
	const std::filesystem::path& GetShaderCacheRootPath();
	const std::filesystem::path& GetShaderDebugArtifactRootPath();
	const std::filesystem::path& GetShaderRecookSignalPath();
	std::filesystem::path BuildShaderRecookSignalPath(const std::filesystem::path& shaderCacheRootPath);

	// Marker-based repository discovery
	std::optional<std::filesystem::path> FindAncestorWithMarker(
	    const std::filesystem::path& startDir,
	    std::string_view markerFileName,
	    uint32_t maxDepth = 32);

	std::optional<std::filesystem::path> DiscoverWorkspaceRoot();

	std::optional<std::filesystem::path> DiscoverEngineRoot();

	std::optional<std::filesystem::path> DiscoverProjectRoot();

	// Source asset roots
	const std::filesystem::path& GetTypedPath(AssetType type, PathRoot root = PathRoot::Any) noexcept;
	const std::filesystem::path& GetShaderPath(PathRoot root = PathRoot::Any) noexcept;
	const std::filesystem::path& GetShaderSymbolsPath(PathRoot root = PathRoot::Any) noexcept;

	// Source asset resolution
	std::optional<std::filesystem::path> ResolveAssetPathNormalized(const std::filesystem::path& inputPath, AssetType type);
	std::optional<std::filesystem::path> ResolveAssetPath(const std::filesystem::path& inputPath, AssetType type);
	std::filesystem::path ResolveAssetPathValidated(const std::filesystem::path& inputPath, AssetType type);
	const std::filesystem::path& GetShaderSymbolsOutputPath();
}  // namespace Filesystem