#pragma once

#include "SparkleLauncher/MaintenanceOperations.h"

namespace SparkleLauncher
{
	enum class MaintenanceCleanBehavior
	{
		RemovePath,
		RemoveBuildDirectoryContentsPreservingDependencies,
		RemoveRootGeneratedFiles
	};

	struct MaintenanceOperationProcessStep
	{
		std::string Id;
		std::string DisplayName;
		ProcessRequest Request;
		std::filesystem::path DestructivePath;
		MaintenanceCleanBehavior CleanBehavior = MaintenanceCleanBehavior::RemovePath;
		bool HasProcessRequest = true;
		bool DeletesGeneratedOutput = false;
	};

	std::vector<MaintenanceOperationProcessStep> BuildMaintenanceProcessStepsForPlan(const MaintenanceOperationPlan& plan);
}