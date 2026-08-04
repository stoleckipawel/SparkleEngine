#pragma once

#include "LauncherCapabilityRegistry.h"

#include <set>
#include <string>

namespace SparkleLauncher
{
	struct LauncherLevelUiModel;

	LauncherCapabilityResolution PlanLauncherQuickStartStep(
	    const LauncherOperationRequest& launchRequest,
	    const LauncherLevelUiModel& levelModel,
	    const std::set<std::string>& invalidatedCapabilityIds);
}
