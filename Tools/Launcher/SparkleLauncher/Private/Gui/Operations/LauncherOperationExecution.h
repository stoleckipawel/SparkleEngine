#pragma once

#include "LauncherOperationRequest.h"
#include "SparkleLauncher/BuildWorkspaceOperations.h"
#include "SparkleLauncher/CookOperations.h"
#include "SparkleLauncher/LaunchOperations.h"
#include "SparkleLauncher/MaintenanceOperations.h"
#include "SparkleLauncher/OperationModel.h"
#include "SparkleLauncher/ProcessRunner.h"

#include <string>
#include <string_view>
#include <variant>

class TaskExecutionContext;

namespace SparkleLauncher
{
	using LauncherOperationPlan =
	    std::variant<BuildWorkspaceOperationPlan, CookOperationPlan, MaintenanceOperationPlan, LaunchOperationPlan>;

	LauncherOperationPlan PlanLauncherOperation(
	    LauncherOperationCategory category,
	    std::string_view operationId,
	    const LauncherOperationRequest& request);

	OperationRecord ExecuteLauncherOperation(
	    LauncherOperationCategory category,
	    std::string operationId,
	    const LauncherOperationRequest& request,
	    IProcessRunner& processRunner,
	    TaskExecutionContext& context,
	    const ProcessOutputCallback& outputCallback);
}
