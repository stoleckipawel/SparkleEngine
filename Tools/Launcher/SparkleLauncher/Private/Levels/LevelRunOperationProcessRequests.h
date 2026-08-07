#pragma once

#include "SparkleLauncher/LevelRunOperations.h"

#include <vector>

namespace SparkleLauncher
{
	struct LevelRunOperationProcessStep
	{
		std::string Id;
		std::string DisplayName;
		ProcessRequest Request;
	};

	std::vector<LevelRunOperationProcessStep> BuildLevelRunProcessStepsForPlan(const LevelRunOperationPlan& plan);
}
