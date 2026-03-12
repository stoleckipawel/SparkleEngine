#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string_view>

namespace Engine::FileSystem
{
	inline constexpr std::string_view kWorkspaceMarker = ".sparkle";
	inline constexpr std::string_view kEngineMarker = ".sparkle-engine";
	inline constexpr std::string_view kProjectMarker = ".sparkle-project";

	std::filesystem::path NormalizePath(const std::filesystem::path& path);

	std::filesystem::path GetExecutableDirectory();

	std::optional<std::filesystem::path> FindAncestorWithMarker(
	    const std::filesystem::path& startDir,
	    std::string_view markerFileName,
	    uint32_t maxDepth = 32);

	std::optional<std::filesystem::path> DiscoverWorkspaceRoot();

	std::optional<std::filesystem::path> DiscoverEngineRoot();

	std::optional<std::filesystem::path> DiscoverProjectRoot();
}