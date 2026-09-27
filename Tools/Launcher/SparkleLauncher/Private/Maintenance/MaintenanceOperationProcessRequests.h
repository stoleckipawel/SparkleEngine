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
		RemoveDirectoryContentsPreservingPath,
		RemoveRootGeneratedFiles
	};

	struct MaintenanceOperationProcessStep
	{
		std::string Id;
		std::string DisplayName;
		std::filesystem::path DestructivePath;
		std::filesystem::path PreservedPath;
		MaintenanceCleanBehavior CleanBehavior = MaintenanceCleanBehavior::RemovePath;
		bool DeletesGeneratedOutput = false;
	};

	std::vector<MaintenanceOperationProcessStep> BuildMaintenanceProcessStepsForPlan(const MaintenanceOperationPlan& plan);
}
