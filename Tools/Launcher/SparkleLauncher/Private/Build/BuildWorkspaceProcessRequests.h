#pragma once

#include "SparkleLauncher/BuildWorkspaceOperations.h"

#include <string>
#include <vector>

namespace SparkleLauncher
{
	struct BuildWorkspaceProcessStep
	{
		std::string Id;
		std::string DisplayName;
		ProcessRequest Request;
		bool UpdatesBuildFilesFreshness = false;
	};

	std::vector<BuildWorkspaceProcessStep> BuildProcessStepsForPlan(const BuildWorkspaceOperationPlan& plan);
	bool BuildWorkspaceExecutionPlanMatches(
	    const BuildWorkspaceOperationPlan& plan,
	    const std::vector<BuildWorkspaceProcessStep>& processSteps);
}
