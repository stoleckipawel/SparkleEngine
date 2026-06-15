#include "LaunchOperationProcessRequests.h"

#include "Smoke/RhiSmokeLaunchOperations.h"
#include "Smoke/RhiSmokePtlasArticleTestPlan.h"
#include "Smoke/RhiSmokeParityTestPlan.h"
#include "Smoke/RhiSmokePtlasBenchmarkTestPlan.h"
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
		if (IsRhiParitySmokeLaunchOperation(plan.Kind))
		{
			return BuildRhiSmokeRayTracingParityProcessSteps(plan);
		}
		if (IsRhiPtlasBenchmarkSmokeLaunchOperation(plan.Kind))
		{
			return BuildRhiSmokePtlasBenchmarkProcessSteps(plan);
		}
		if (IsRhiPtlasArticleSmokeLaunchOperation(plan.Kind))
		{
			return BuildRhiSmokePtlasArticleProcessSteps(plan);
		}

		ProcessRequest request;
		request.ExecutablePath = plan.ExecutablePath;
		request.WorkingDirectory = plan.WorkingDirectory;
		request.Environment = plan.Environment;
		request.LogPath = GetLauncherOperationLogPath(plan.RepositoryRoot, plan.Operation.Id, "Launch.txt");
		if (!plan.Request.GraphicsBackend.empty())
		{
			request.Arguments.push_back("--graphics-api");
			request.Arguments.push_back(plan.Request.GraphicsBackend);
		}
		if (!plan.Request.VSync.empty())
		{
			request.Arguments.push_back("--cvar");
			request.Arguments.push_back("r.VSync=" + plan.Request.VSync);
		}
		if (!plan.Request.PreferHighPerformanceAdapter.empty())
		{
			request.Arguments.push_back("--cvar");
			request.Arguments.push_back("r.PreferHighPerformanceAdapter=" + plan.Request.PreferHighPerformanceAdapter);
		}
		if (!plan.Request.MeshAutoBatching.empty())
		{
			request.Arguments.push_back("--cvar");
			request.Arguments.push_back("r.MeshAutoBatching=" + plan.Request.MeshAutoBatching);
		}
		for (const std::string& customArgument : plan.Request.CustomArguments)
		{
			if (!customArgument.empty())
			{
				request.Arguments.push_back(customArgument);
			}
		}
		for (const std::string& customCVar : plan.Request.CustomCVars)
		{
			if (!customCVar.empty())
			{
				request.Arguments.push_back("--cvar");
				request.Arguments.push_back(customCVar);
			}
		}

		LaunchOperationProcessStep step;
		step.Id = "launch";
		step.DisplayName = "Launch " + plan.TargetName;
		step.Request = std::move(request);
		steps.push_back(std::move(step));
		return steps;
	}
}
