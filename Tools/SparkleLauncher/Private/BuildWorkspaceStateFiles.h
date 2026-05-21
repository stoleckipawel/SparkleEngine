#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

namespace SparkleLauncher
{
	std::optional<std::string> ReadCMakeCacheValue(const std::filesystem::path& cachePath, std::string_view key);
	std::optional<std::string> ReadBuildFilesFreshnessStampValue(const std::filesystem::path& stampPath, std::string_view key);
	std::optional<std::string> ReadRootCMakeProjectName(const std::filesystem::path& repositoryRoot);
	std::filesystem::path GetBuildSolutionPath(const std::filesystem::path& repositoryRoot);
	std::string BuildUtcTimestamp();
}
