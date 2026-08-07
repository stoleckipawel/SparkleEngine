#include "LevelRunOperationProcessRequests.h"

#include "SparkleLauncher/LauncherPaths.h"

#include <utility>

namespace SparkleLauncher
{
	std::vector<LevelRunOperationProcessStep> BuildLevelRunProcessStepsForPlan(const LevelRunOperationPlan& plan)
	{
		std::vector<LevelRunOperationProcessStep> steps;
		if (!plan.CanRun)
		{
			return steps;
		}

		ProcessRequest request;
		request.ExecutablePath = plan.ExecutablePath;
		request.WorkingDirectory = plan.WorkingDirectory;
		request.Environment = plan.Environment;
		request.LogPath = GetLauncherOperationLogPath(plan.RepositoryRoot, plan.Operation.Id, "RunLevel.txt");
		request.Arguments = {"--graphics-api", plan.Request.GraphicsApi};

		LevelRunOperationProcessStep step;
		step.Id = "run-level";
		step.DisplayName = "Run " + plan.Request.LevelId;
		step.Request = std::move(request);
		steps.push_back(std::move(step));
		return steps;
	}
}
