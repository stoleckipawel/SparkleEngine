#pragma once

#include "SparkleLauncher/LaunchOperations.h"
#include "Smoke/RhiSmokeTestCatalog.h"

#include <string>
#include <vector>

namespace SparkleLauncher
{
	bool IsRhiSmokeLaunchOperation(LaunchOperationKind kind);
	bool IsRhiParitySmokeLaunchOperation(LaunchOperationKind kind);
	bool IsRhiSmokeTestEnabled(const LaunchOperationPlan& plan) noexcept;
	std::string GetRhiSmokeFrameLimitText(const LaunchOperationRequest& request);
	void PopulateRhiSmokeLaunchInputs(LaunchOperationPlan& plan);
	void PopulateRhiSmokeLaunchEnvironment(LaunchOperationPlan& plan);
	std::vector<std::string> GetRhiSmokeLaunchPlannedEffects(const LaunchOperationPlan& plan);
}
