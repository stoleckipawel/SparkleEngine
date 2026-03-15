#pragma once

#include "GameFramework/Public/Assets/AssetTypes.h"
#include "GameFramework/Public/Paths/PathRoot.h"

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string_view>

namespace Filesystem
{
	inline constexpr std::string_view kWorkspaceMarker = ".sparkle";
	inline constexpr std::string_view kEngineMarker = ".sparkle-engine";
	inline constexpr std::string_view kProjectMarker = ".sparkle-project";

	std::filesystem::path NormalizePath(const std::filesystem::path& path);

	const std::filesystem::path& GetWorkingDirectory();
	std::filesystem::path GetExecutableDirectory();
	const std::filesystem::path& GetProjectPath();
	const std::filesystem::path& GetProjectAssetsPath();
	const std::filesystem::path& GetEnginePath();
	const std::filesystem::path& GetEngineAssetsPath();

	std::optional<std::filesystem::path> FindAncestorWithMarker(
	    const std::filesystem::path& startDir,
	    std::string_view markerFileName,
	    uint32_t maxDepth = 32);

	std::optional<std::filesystem::path> DiscoverWorkspaceRoot();

	std::optional<std::filesystem::path> DiscoverEngineRoot();

	std::optional<std::filesystem::path> DiscoverProjectRoot();

	const std::filesystem::path& GetTypedPath(AssetType type, PathRoot root = PathRoot::Any) noexcept;
	const std::filesystem::path& GetShaderPath(PathRoot root = PathRoot::Any) noexcept;
	const std::filesystem::path& GetShaderSymbolsPath(PathRoot root = PathRoot::Any) noexcept;
	const std::filesystem::path& GetTexturePath(PathRoot root = PathRoot::Any) noexcept;
	const std::filesystem::path& GetMeshPath(PathRoot root = PathRoot::Any) noexcept;

	std::optional<std::filesystem::path> ResolveAssetPath(const std::filesystem::path& inputPath, AssetType type);
	std::filesystem::path ResolveAssetPathValidated(const std::filesystem::path& inputPath, AssetType type);
	const std::filesystem::path& GetShaderSymbolsOutputPath();

	bool HasProjectAssets() noexcept;
	bool HasEngineAssets() noexcept;
}  // namespace Filesystem