#pragma once

#include <filesystem>
#include <optional>
#include <string>

namespace SparkleLauncher
{
	struct RepositoryRoot
	{
		std::filesystem::path RootPath;
		std::filesystem::path EnginePath;
		std::filesystem::path ToolsPath;
		std::filesystem::path ContentPath;
	};

	bool IsRepositoryRoot(const std::filesystem::path& path);
	std::optional<RepositoryRoot> TryFindRepositoryRoot(const std::filesystem::path& startPath, std::string& outErrorMessage);
}
