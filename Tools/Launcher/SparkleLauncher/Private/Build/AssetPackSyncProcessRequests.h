#pragma once

#include "BuildWorkspaceProcessRequests.h"

namespace SparkleLauncher
{
	void AppendAssetPackSyncProcessSteps(std::vector<BuildWorkspaceProcessStep>& steps, const BuildWorkspaceOperationPlan& plan);
}
