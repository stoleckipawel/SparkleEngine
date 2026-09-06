#pragma once

#include "SparkleLauncher/LevelOperations.h"

#include <string>
#include <vector>

namespace SparkleLauncher
{
	struct LevelOperationProcessStep
	{
		std::string Id;
		std::string DisplayName;
		ProcessRequest Request;
	};

	std::vector<LevelOperationProcessStep> BuildLevelOperationProcessSteps(const LevelOperationPlan& plan);
	bool LevelOperationExecutionPlanMatches(const LevelOperationPlan& plan, const std::vector<LevelOperationProcessStep>& processSteps);
}
