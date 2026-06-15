#pragma once

#include "LaunchOperationProcessRequests.h"

#include <vector>

namespace SparkleLauncher
{
	std::vector<LaunchOperationProcessStep> BuildRhiSmokePtlasBenchmarkProcessSteps(const LaunchOperationPlan& plan);
}
