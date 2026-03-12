// ============================================================================
// FileSystemUtils.h
// Platform-agnostic filesystem utilities and marker-based root discovery.
// ----------------------------------------------------------------------------
// USAGE:
//   auto normalized = Engine::FileSystem::NormalizePath(rawPath);
//   auto exeDir = Engine::FileSystem::GetExecutableDirectory();
//   auto workspaceRoot = Engine::FileSystem::DiscoverWorkspaceRoot();
//
// DESIGN:
//   Marker files identify directory roles:
//     .sparkle         - Workspace root (repository level)
//     .sparkle-engine  - Engine root (engine/ subdirectory)
//     .sparkle-project - Project root (each game/sample project)
// ============================================================================
#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string_view>

namespace Engine::FileSystem
{

	// =========================================================================
	// Marker Files
	// =========================================================================

	inline constexpr std::string_view kWorkspaceMarker = ".sparkle";
	inline constexpr std::string_view kEngineMarker = ".sparkle-engine";
	inline constexpr std::string_view kProjectMarker = ".sparkle-project";

	// =========================================================================
	// Path Normalization
	// =========================================================================

	std::filesystem::path NormalizePath(const std::filesystem::path& path);

	// =========================================================================
	// Directory Queries
	// =========================================================================

	std::filesystem::path GetExecutableDirectory();

	// =========================================================================
	// Marker-Based Discovery
	// =========================================================================

	// Walks up directory tree from startDir looking for a marker file.
	std::optional<std::filesystem::path> FindAncestorWithMarker(
	    const std::filesystem::path& startDir,
	    std::string_view markerFileName,
	    uint32_t maxDepth = 32);

	// Discovers workspace root (.sparkle marker).
	std::optional<std::filesystem::path> DiscoverWorkspaceRoot();

	// Discovers engine root (.sparkle-engine marker).
	std::optional<std::filesystem::path> DiscoverEngineRoot();

	// Discovers project root (.sparkle-project marker).
	std::optional<std::filesystem::path> DiscoverProjectRoot();

}  // namespace Engine::FileSystem
