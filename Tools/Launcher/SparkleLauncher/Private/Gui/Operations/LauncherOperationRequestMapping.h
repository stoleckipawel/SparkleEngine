#pragma once

#include "LauncherOperationRequest.h"
#include "SparkleLauncher/BuildWorkspaceOperations.h"
#include "SparkleLauncher/CookOperations.h"
#include "SparkleLauncher/LevelOperations.h"
#include "SparkleLauncher/LaunchOperations.h"
#include "SparkleLauncher/MaintenanceOperations.h"

namespace SparkleLauncher::LauncherOperationRequestMapping
{
	BuildWorkspaceOperationRequest BuildWorkspace(const LauncherOperationRequest& request);
	LevelOperationRequest Levels(const LauncherOperationRequest& request);
	CookOperationRequest Cook(const LauncherOperationRequest& request);
	MaintenanceOperationRequest Maintenance(const LauncherOperationRequest& request);
	LaunchOperationRequest Launch(const LauncherOperationRequest& request);
}
