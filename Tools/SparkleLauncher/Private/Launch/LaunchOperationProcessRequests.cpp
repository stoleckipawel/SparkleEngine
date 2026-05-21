#include "LaunchOperationProcessRequests.h"

#include "SparkleLauncher/LauncherPaths.h"

#include <utility>

namespace SparkleLauncher
{
	std::vector<LaunchOperationProcessStep> BuildLaunchProcessStepsForPlan(const LaunchOperationPlan& plan)
	{
		std::vector<LaunchOperationProcessStep> steps;
		if (!plan.CanRun)
		{
			return steps;
		}

		ProcessRequest request;
		request.ExecutablePath = plan.ExecutablePath;
		request.WorkingDirectory = plan.WorkingDirectory;
		request.LogPath = GetLauncherOperationLogPath(plan.RepositoryRoot, plan.Operation.Id, "Launch.txt");

		LaunchOperationProcessStep step;
		step.Id = "launch";
		step.DisplayName = "Launch " + plan.TargetName;
		step.Request = std::move(request);
		steps.push_back(std::move(step));
		return steps;
	}
}