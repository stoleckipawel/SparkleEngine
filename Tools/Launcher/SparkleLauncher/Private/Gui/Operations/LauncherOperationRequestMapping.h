#pragma once

#include "LauncherOperationRequest.h"
#include "SparkleLauncher/BuildWorkspaceOperations.h"
#include "SparkleLauncher/CookOperations.h"
#include "SparkleLauncher/LevelOperations.h"
#include "SparkleLauncher/LevelRunOperations.h"
#include "SparkleLauncher/MaintenanceOperations.h"

namespace SparkleLauncher::LauncherOperationRequestMapping
{
	bool RequestsLauncherRebuild(const LauncherOperationRequest& request);
	BuildWorkspaceOperationRequest BuildWorkspace(const LauncherOperationRequest& request);
	LevelOperationRequest Levels(const LauncherOperationRequest& request);
	LevelRunOperationRequest LevelRun(const LauncherOperationRequest& request);
	CookOperationRequest Cook(const LauncherOperationRequest& request);
	MaintenanceOperationRequest Maintenance(const LauncherOperationRequest& request);
}
