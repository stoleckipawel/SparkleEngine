#include "Smoke/RhiSmokeLaunchOperations.h"

#include <utility>

namespace SparkleLauncher
{
	bool IsRhiSmokeLaunchOperation(LaunchOperationKind kind)
	{
		return kind == LaunchOperationKind::RunProject;
	}

	std::string GetRhiSmokeFrameLimitText(const LaunchOperationRequest& request)
	{
		return request.SmokeFrameLimit.empty() ? std::string("120") : request.SmokeFrameLimit;
	}

	void PopulateRhiSmokeLaunchInputs(LaunchOperationPlan& plan)
	{
		if (!IsRhiSmokeLaunchOperation(plan.Kind))
		{
			return;
		}
		if (!plan.Request.EnableSmokeTest)
		{
			return;
		}

		plan.Operation.Inputs.push_back({"smokeBackend", plan.Request.SmokeBackend.empty() ? "default" : plan.Request.SmokeBackend});
		plan.Operation.Inputs.push_back({"smokeFrameLimit", GetRhiSmokeFrameLimitText(plan.Request)});
	}

	static void AddEnvironment(LaunchOperationPlan& plan, std::string name, std::string value)
	{
		EnvironmentOverride overrideValue;
		overrideValue.Name = std::move(name);
		overrideValue.Value = std::move(value);
		plan.Environment.push_back(std::move(overrideValue));
	}

	void PopulateRhiSmokeLaunchEnvironment(LaunchOperationPlan& plan)
	{
		if (!IsRhiSmokeLaunchOperation(plan.Kind))
		{
			return;
		}
		if (!plan.Request.EnableSmokeTest)
		{
			return;
		}

		AddEnvironment(plan, "SPARKLE_SMOKE_VALIDATE_RHI", "1");
		AddEnvironment(plan, "SPARKLE_SMOKE_FRAME_LIMIT", GetRhiSmokeFrameLimitText(plan.Request));
		if (!plan.Request.SmokeBackend.empty())
		{
			AddEnvironment(plan, "SPARKLE_RHI_BACKEND", plan.Request.SmokeBackend);
		}
		if (plan.Request.SmokeTrace)
		{
			AddEnvironment(plan, "SPARKLE_SMOKE_TRACE", "1");
		}
		if (plan.Request.SmokeSkipLevelSwitching)
		{
			AddEnvironment(plan, "SPARKLE_SMOKE_SKIP_LEVEL_SWITCHING", "1");
		}
	}

	std::vector<std::string> GetRhiSmokeLaunchPlannedEffects(const LaunchOperationPlan& plan)
	{
		if (!IsRhiSmokeLaunchOperation(plan.Kind))
		{
			return {};
		}
		if (!plan.Request.EnableSmokeTest)
		{
			return {};
		}

		return {"Enable graphics smoke validation for " + GetRhiSmokeFrameLimitText(plan.Request) + " frames."};
	}
}
