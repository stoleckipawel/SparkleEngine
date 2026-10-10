#include "LevelRunOperationProcessRequests.h"

#include "LauncherStatePaths.h"

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
		request.LogPath = ResolveLauncherOperationLogPath(plan.RepositoryRoot, plan.Operation.Id, "RunLevel.txt");
		request.Arguments = {"--graphics-api", plan.Request.GraphicsApi};
		if (plan.Request.ProductProfile == "DebugEditor" || plan.Request.ProductProfile == "DevelopmentEditor")
		{
			request.Arguments.emplace_back("--capture-provider");
			request.Arguments.emplace_back(ExternalCaptureProviderToString(plan.Request.CaptureProvider));
		}
		request.ReadinessValue = plan.Request.LevelId;

		LevelRunOperationProcessStep step;
		step.Id = "run-level";
		step.DisplayName = (plan.Request.RunMode == LevelRunMode::Editor ? "Open " : "Run ") + plan.Request.LevelId;
		step.Request = std::move(request);
		steps.push_back(std::move(step));
		return steps;
	}
}
