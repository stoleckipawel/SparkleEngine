#pragma once

#include "LauncherCapabilityRegistry.h"

namespace SparkleLauncher
{
	struct LauncherLevelUiModel;

	LauncherCapabilityResolution PlanLauncherQuickStartStep(const LauncherOperationRequest& launchRequest, const LauncherLevelUiModel& levelModel);
}
