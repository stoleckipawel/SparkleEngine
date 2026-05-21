#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace SparkleLauncher
{
	struct SparkleProject
	{
		std::string Id;
		std::string DisplayName;
		std::filesystem::path RootPath;
		std::filesystem::path MarkerPath;
	};

	std::vector<SparkleProject> DiscoverProjects(const std::filesystem::path& repositoryRoot, std::string& outErrorMessage);
	std::filesystem::path GetProjectsDirectory(const std::filesystem::path& repositoryRoot);
}