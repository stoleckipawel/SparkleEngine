#include "SparkleLauncher/BuildProfileCatalog.h"
#include "SparkleLauncher/LauncherPaths.h"
#include "SparkleLauncher/ProcessRunner.h"
#include "SparkleLauncher/ProjectDiscovery.h"
#include "SparkleLauncher/RepositoryLocator.h"
#include "Core/Public/Threading/ThreadOwnership.h"
#include "Core/Public/Diagnostics/Error.h"
#include "Core/Public/Projects/ProjectLevelCatalog.h"

#include <filesystem>
#include <iostream>
#include <optional>
#include <vector>

int main(int argc, char** argv)
{
	Threading::SetCurrentThreadRole("Sparkle.ToolMain");
	std::string errorMessage;
	const std::filesystem::path startPath = argc > 1 ? std::filesystem::path(argv[1]) : std::filesystem::current_path();
	const std::optional<SparkleLauncher::RepositoryRoot> repositoryRoot = SparkleLauncher::TryFindRepositoryRoot(startPath, errorMessage);
	if (!repositoryRoot.has_value())
	{
		std::cerr << errorMessage << '\n';
		return 1;
	}

	std::cout << "Repository: " << repositoryRoot->RootPath.string() << '\n';
	std::cout << "Launcher state: " << SparkleLauncher::GetLauncherStateDirectory(repositoryRoot->RootPath).string() << '\n';

	std::vector<SparkleLauncher::SparkleProject> projects = SparkleLauncher::DiscoverProjects(repositoryRoot->RootPath, errorMessage);
	if (!errorMessage.empty())
	{
		std::cerr << errorMessage << '\n';
		return 1;
	}

	std::cout << "Projects:" << '\n';
	for (const SparkleLauncher::SparkleProject& project : projects)
	{
		std::cout << "  " << project.Id << " -> " << project.RootPath.string() << '\n';
		try
		{
			const ProjectLevelCatalog catalog = ProjectLevelCatalogFile::Load(project.RootPath);
			std::cout << "    " << catalog.levels.size() << " levels, " << catalog.optionalContentPacks.size() << " optional content packs"
			          << '\n';
		}
		catch (const Diagnostics::Error& error)
		{
			std::cerr << error.what() << '\n';
			return 1;
		}
	}

	std::cout << "Profiles:" << '\n';
	for (const SparkleLauncher::BuildProfile& profile : SparkleLauncher::GetBuildProfileCatalog())
	{
		std::cout << "  " << profile.Name << " [" << SparkleLauncher::ToString(profile.State) << ", "
		          << SparkleLauncher::ToString(profile.Target) << "]" << '\n';
	}

	return 0;
}
