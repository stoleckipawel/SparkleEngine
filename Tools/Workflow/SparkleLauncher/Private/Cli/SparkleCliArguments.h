#pragma once

#include "SparkleLauncher/CookOperations.h"
#include "SparkleLauncher/MaintenanceOperations.h"

#include <filesystem>
#include <string>
#include <vector>

namespace SparkleLauncher
{
	struct SparkleCliArguments
	{
		std::filesystem::path StartPath;
		std::string OperationId;
		std::string ProjectId;
		std::string EditorProfile = "DevelopmentEditor";
		std::string RuntimeProfile = "DevelopmentGame";
		std::vector<std::string> SelectedTargets;
		std::vector<std::string> ShaderPackages;
		std::vector<std::string> ValidationGroups;
		std::vector<std::string> ValidationTargets;
		std::string SmokeBackend;
		std::string SmokeFrameLimit;
		CookMode RequestedCookMode = CookMode::Incremental;
		FormatMode RequestedFormatMode = FormatMode::Check;
		CleanScope RequestedCleanScope = CleanScope::SelectedProjectCookedOutputs;
		bool DryRun = false;
		bool ForceConfigure = false;
		bool ForceRecookConfirmed = false;
		bool CleanConfirmed = false;
		bool SmokeTrace = false;
		bool SmokeSkipLevelSwitching = false;
		bool ListOperations = false;
		bool ListValidationTargets = false;
		bool ShowHelp = false;
	};
}