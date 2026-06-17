#pragma once

#include "SparkleLauncher/LaunchOperations.h"

#include <string>

namespace SparkleLauncher
{
	bool ValidateRhiSmokeScenarioArtifacts(const LaunchOperationPlan& plan, std::string& outFailureSummary);
}
