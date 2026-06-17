#pragma once

#include "LaunchOperationProcessRequests.h"

#include <vector>

namespace SparkleLauncher
{
	std::vector<LaunchOperationProcessStep> BuildRhiSmokeScenarioProcessSteps(const LaunchOperationPlan& plan);
}
