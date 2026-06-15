#pragma once

#include "LaunchOperationProcessRequests.h"

#include <vector>

namespace SparkleLauncher
{
	std::vector<LaunchOperationProcessStep> BuildRhiSmokePtlasArticleProcessSteps(const LaunchOperationPlan& plan);
}
