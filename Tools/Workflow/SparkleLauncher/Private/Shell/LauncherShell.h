#pragma once

#include "SparkleLauncher/BuildProfileCatalog.h"
#include "SparkleLauncher/CookOperations.h"
#include "SparkleLauncher/LaunchOperations.h"
#include "SparkleLauncher/MaintenanceOperations.h"
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
		std::string SmokeBackend;
		std::string SmokeFrameLimit;
		CookMode RequestedCookMode = CookMode::Incremental;
		FormatMode RequestedFormatMode = FormatMode::Check;
		CleanScope RequestedCleanScope = CleanScope::SelectedProjectCookedOutputs;
		bool ForceRecookConfirmed = false;
		bool CleanConfirmed = false;
		bool SmokeTrace = false;
		bool SmokeSkipLevelSwitching = false;
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