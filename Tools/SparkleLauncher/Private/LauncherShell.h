#pragma once

#include "SparkleLauncher/BuildProfileCatalog.h"
#include "SparkleLauncher/CookOperations.h"
#include "SparkleLauncher/OperationModel.h"
#include "SparkleLauncher/ProjectDiscovery.h"
#include "SparkleLauncher/RepositoryLocator.h"

#include <filesystem>
#include <iosfwd>
#include <string>
#include <vector>

namespace SparkleLauncher
{
	struct LauncherShellArguments
	{
		std::filesystem::path StartPath;
		std::string SelectedProject;
		std::string EditorProfile = "DevelopmentEditor";
		std::string RuntimeProfile = "DevelopmentGame";
		std::string DryRunOperationId;
		CookMode RequestedCookMode = CookMode::Incremental;
		bool ForceRecookConfirmed = false;
		bool ShowHelp = false;
	};

	class LauncherShell final
	{
	public:
		int Run(int argc, char** argv, std::ostream& output, std::ostream& error) const;

	private:
		bool ParseArguments(int argc, char** argv, LauncherShellArguments& outArguments, std::ostream& error) const;
		void PrintUsage(std::ostream& output) const;
	};
}