#pragma once

#include "BuildWorkspaceProcessRequests.h"

namespace SparkleLauncher
{
	void AppendOptionalContentSyncProcessSteps(std::vector<BuildWorkspaceProcessStep>& steps, const BuildWorkspaceOperationPlan& plan);
}
