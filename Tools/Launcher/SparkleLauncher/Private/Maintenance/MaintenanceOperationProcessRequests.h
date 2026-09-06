#pragma once

#include "SparkleLauncher/MaintenanceOperations.h"

#include <filesystem>
#include <string>
#include <vector>
#include <cstdint>

namespace SparkleLauncher
{
	enum class MaintenanceCleanBehavior : std::uint8_t
	{
		RemovePath,
		RemoveBuildDirectoryContentsPreservingDependencies,
		RemoveRootGeneratedFiles
	};

	struct MaintenanceOperationProcessStep
	{
		std::string Id;
		std::string DisplayName;
		std::filesystem::path DestructivePath;
		MaintenanceCleanBehavior CleanBehavior = MaintenanceCleanBehavior::RemovePath;
		bool DeletesGeneratedOutput = false;
	};

	std::vector<MaintenanceOperationProcessStep> BuildMaintenanceProcessStepsForPlan(const MaintenanceOperationPlan& plan);
}
