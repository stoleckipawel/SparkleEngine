#pragma once

#include "SparkleLauncher/LaunchOperations.h"

namespace SparkleLauncher
{
	struct LaunchOperationProcessStep
	{
		std::string Id;
		std::string DisplayName;
		ProcessRequest Request;
	};

	std::vector<LaunchOperationProcessStep> BuildLaunchProcessStepsForPlan(const LaunchOperationPlan& plan);
}