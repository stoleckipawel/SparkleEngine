#pragma once

#include "SparkleLauncher/OperationModel.h"
#include "SparkleLauncher/ProcessRunner.h"

#include <string>

class TaskExecutionContext;

namespace SparkleLauncher
{
	enum class LauncherOperationCategory;
	struct LauncherOperationRequest;

	OperationRecord ExecuteLauncherOperation(
	    LauncherOperationCategory category,
	    std::string operationId,
	    std::string title,
	    const LauncherOperationRequest& request,
	    IProcessRunner& processRunner,
	    TaskExecutionContext& context,
	    const ProcessOutputCallback& outputCallback);
}
