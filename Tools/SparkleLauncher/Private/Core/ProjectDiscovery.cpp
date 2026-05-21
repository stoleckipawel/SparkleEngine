#include "SparkleLauncher/ProjectDiscovery.h"

#include <algorithm>
#include <system_error>

namespace SparkleLauncher
{
	std::vector<SparkleProject> DiscoverProjects(const std::filesystem::path& repositoryRoot, std::string& outErrorMessage)
	{
		outErrorMessage.clear();

		const std::filesystem::path projectsDirectory = GetProjectsDirectory(repositoryRoot);
		std::error_code errorCode;
		if (!std::filesystem::exists(projectsDirectory, errorCode) || !std::filesystem::is_directory(projectsDirectory, errorCode))
		{
			outErrorMessage = "Projects directory was not found: " + projectsDirectory.string();
			return {};
		}

		std::filesystem::directory_iterator projectIterator(projectsDirectory, errorCode);
		if (errorCode)
		{
			outErrorMessage = "Failed to enumerate projects: " + errorCode.message();
			return {};
		}

		std::vector<SparkleProject> projects;
		for (const std::filesystem::directory_entry& entry : projectIterator)
		{
			if (errorCode)
			{
				outErrorMessage = "Failed to enumerate projects: " + errorCode.message();
				return projects;
			}

			if (!entry.is_directory(errorCode))
			{
				continue;
			}

			const std::filesystem::path markerPath = entry.path() / ".sparkle-project";
			if (!std::filesystem::exists(markerPath, errorCode))
			{
				continue;
			}

			SparkleProject project;
			project.RootPath = entry.path();
			project.MarkerPath = markerPath;
			project.Id = entry.path().filename().string();
			project.DisplayName = project.Id;
			projects.push_back(std::move(project));
		}

		std::sort(projects.begin(), projects.end(), [](const SparkleProject& left, const SparkleProject& right) {
			return left.Id < right.Id;
		});

		return projects;
	}

	std::filesystem::path GetProjectsDirectory(const std::filesystem::path& repositoryRoot)
	{
		return repositoryRoot / "Projects";
	}
}