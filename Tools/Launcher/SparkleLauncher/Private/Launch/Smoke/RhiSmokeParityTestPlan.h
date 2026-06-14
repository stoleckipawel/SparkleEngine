#pragma once

#include "LaunchOperationProcessRequests.h"

#include <vector>

namespace SparkleLauncher
{
	std::vector<LaunchOperationProcessStep> BuildRhiSmokeRayTracingParityProcessSteps(const LaunchOperationPlan& plan);
}
