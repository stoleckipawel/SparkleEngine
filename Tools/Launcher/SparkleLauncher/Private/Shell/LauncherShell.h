#pragma once

#include "SparkleLauncher/BuildWorkspaceOperations.h"
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
		WorkspaceIde WorkspaceIdePreference = WorkspaceIde::VisualStudio;
		std::string DryRunOperationId;
		std::string RunOperationId;
		std::string LaunchTarget = "editor";
		std::string LaunchStartupLevel = "Sponza";
		bool EnableSmokeTest = false;
		std::string SmokeBackend;
		std::string SmokeFrameLimit;
		std::string SmokeViewMode;
		std::string SmokeCapturePath;
		CookMode RequestedCookMode = CookMode::Incremental;
		FormatMode RequestedFormatMode = FormatMode::Check;
		CleanScope RequestedCleanScope = CleanScope::SelectedProjectCookedOutputs;
		bool ForceRecookConfirmed = false;
		bool CleanConfirmed = false;
		bool SmokeSkipLevelSwitching = false;
		bool SmokeRunRayTracingParity = false;
		bool SmokeRunPtlasBenchmark = false;
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
