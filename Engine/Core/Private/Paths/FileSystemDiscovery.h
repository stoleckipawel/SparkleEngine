#pragma once

#include <filesystem>
#include <optional>
#include <string>

namespace Filesystem::Private
{
	std::string GetExecutableStem();
	std::string InferProjectNameFromExecutableStem(std::string executableStem);
	std::optional<std::filesystem::path> DiscoverPackageRoot();
	std::optional<std::filesystem::path> DiscoverPackageProjectRoot(const std::filesystem::path& packageRoot);
	std::optional<std::filesystem::path> DiscoverWorkspaceProjectRoot();
}
