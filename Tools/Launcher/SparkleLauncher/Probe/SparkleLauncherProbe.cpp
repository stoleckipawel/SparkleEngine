#include "SparkleLauncher/BuildProfileCatalog.h"
#include "SparkleLauncher/LauncherPaths.h"
#include "SparkleLauncher/ProcessRunner.h"
#include "SparkleLauncher/ContentDiscovery.h"
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

	const std::optional<SparkleLauncher::SparkleContent> content =
	    SparkleLauncher::DiscoverContentRoot(repositoryRoot->RootPath, errorMessage);
	if (!content.has_value())
	{
		std::cerr << errorMessage << '\n';
		return 1;
	}

	std::cout << "Content: " << content->RootPath.string() << '\n';
	try
	{
		const ProjectLevelCatalog catalog = ProjectLevelCatalogFile::Load(content->RootPath);
		std::cout << "  " << catalog.levels.size() << " levels, " << catalog.assetPacks.size() << " asset packs" << '\n';
	}
	catch (const Diagnostics::Error& error)
	{
		std::cerr << error.what() << '\n';
		return 1;
	}

	std::cout << "Profiles:" << '\n';
	for (const SparkleLauncher::BuildProfile& profile : SparkleLauncher::GetBuildProfileCatalog())
	{
		std::cout << "  " << profile.Name << " [" << SparkleLauncher::ToString(profile.State) << ", "
		          << SparkleLauncher::ToString(profile.Target) << "]" << '\n';
	}

	return 0;
}
