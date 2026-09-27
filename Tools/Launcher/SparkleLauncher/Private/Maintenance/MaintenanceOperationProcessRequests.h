#pragma once

#include "SparkleLauncher/MaintenanceOperations.h"

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

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
		std::string PreviewDetail;
		MaintenanceCleanBehavior CleanBehavior = MaintenanceCleanBehavior::RemovePath;
		bool DeletesGeneratedOutput = false;
	};

	std::vector<CleanScope> ResolveRequestedCleanScopes(const MaintenanceOperationRequest& request);
	std::vector<MaintenanceOperationProcessStep> BuildMaintenanceCleanSteps(const MaintenanceOperationPlan& plan);
	std::vector<MaintenanceOperationProcessStep> BuildMaintenanceProcessStepsForPlan(const MaintenanceOperationPlan& plan);
}
