#pragma once

#include "SparkleLauncher/BuildWorkspaceOperations.h"

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
}
