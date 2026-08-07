#pragma once

#include "SparkleLauncher/BuildWorkspaceOperations.h"
#include "SparkleLauncher/CookOperations.h"
#include "SparkleLauncher/MaintenanceOperations.h"

#include <filesystem>
#include <iosfwd>
#include <string>

namespace SparkleLauncher
{
	struct LauncherShellArguments final
	{
		std::filesystem::path StartPath;
		std::string EditorProfile = "DevelopmentEditor";
		std::string RuntimeProfile = "DevelopmentGame";
		WorkspaceIde WorkspaceIdePreference = WorkspaceIde::VisualStudio;
		WorkspaceCompiler WorkspaceCompilerPreference = WorkspaceCompiler::Msvc;
		std::string DryRunOperationId;
		std::string RunOperationId;
		CookMode RequestedCookMode = CookMode::Incremental;
		CleanScope RequestedCleanScope = CleanScope::CookedOutputs;
		bool ForceRecookConfirmed = false;
		bool CleanConfirmed = false;
		bool ShowHelp = false;
	};

	bool ParseLauncherShellArguments(int argc, char** argv, LauncherShellArguments& outArguments, std::ostream& error);
	void PrintLauncherShellUsage(std::ostream& output);
}
