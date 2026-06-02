#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace SparkleLauncher
{
	std::vector<std::filesystem::path> CollectBuildInputPaths(const std::filesystem::path& repositoryRoot);
	std::optional<std::string> ComputeSourceListHash(const std::filesystem::path& repositoryRoot);
}
