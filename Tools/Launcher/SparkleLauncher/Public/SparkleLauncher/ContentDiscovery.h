#pragma once

#include <filesystem>
#include <optional>
#include <string>

namespace SparkleLauncher
{
	struct SparkleContent
	{
		std::string Id;
		std::string DisplayName;
		std::filesystem::path RootPath;
		std::filesystem::path MarkerPath;
	};

	std::optional<SparkleContent> DiscoverContentRoot(const std::filesystem::path& repositoryRoot, std::string& outErrorMessage);
}
