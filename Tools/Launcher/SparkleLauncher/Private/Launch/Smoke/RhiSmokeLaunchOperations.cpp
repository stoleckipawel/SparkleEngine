#include "Smoke/RhiSmokeLaunchOperations.h"

#include "Smoke/RhiSmokeTestCatalog.h"

#include <utility>

namespace SparkleLauncher
{
	bool IsRhiSmokeLaunchOperation(LaunchOperationKind kind)
	{
		return kind == LaunchOperationKind::RunProject;
	}

	bool IsRhiSmokeTestEnabled(const LaunchOperationPlan& plan) noexcept
	{
		return plan.Request.EnableSmokeTest;
	}

	bool HasRhiSmokeScenarioMatrix(const LaunchOperationPlan& plan) noexcept
	{
		for (const RhiSmokeSuite suite : GetEnabledRhiSmokeSuites(plan))
		{
			if (suite != RhiSmokeSuite::SingleViewportCapture)
			{
				return true;
			}
		}

		return false;
	}

	std::string GetRhiSmokeFrameLimitText(const LaunchOperationRequest& request)
	{
		return request.SmokeFrameLimit.empty() ? std::string("120") : request.SmokeFrameLimit;
	}

	static void AddEnvironment(LaunchOperationPlan& plan, std::string name, std::string value)
	{
		EnvironmentOverride overrideValue;
		overrideValue.Name = std::move(name);
		overrideValue.Value = std::move(value);
		plan.Environment.push_back(std::move(overrideValue));
	}

	void PopulateRhiSmokeLaunchInputs(LaunchOperationPlan& plan)
	{
		if (!IsRhiSmokeLaunchOperation(plan.Kind) || !IsRhiSmokeTestEnabled(plan))
		{
			return;
		}

		plan.Operation.Inputs.push_back({"smokeSuites", GetRhiSmokeSuiteSummary(plan)});
		plan.Operation.Inputs.push_back({"smokeBackend", plan.Request.SmokeBackend.empty() ? "default" : plan.Request.SmokeBackend});
		plan.Operation.Inputs.push_back({"smokeFrameLimit", GetRhiSmokeFrameLimitText(plan.Request)});
		plan.Operation.Inputs.push_back({"smokeViewMode", plan.Request.SmokeViewMode.empty() ? "default" : plan.Request.SmokeViewMode});
		plan.Operation.Inputs.push_back({"smokeCapturePath", plan.Request.SmokeCapturePath.empty() ? "disabled" : plan.Request.SmokeCapturePath});
	}

	void PopulateRhiSmokeLaunchEnvironment(LaunchOperationPlan& plan)
	{
		if (!IsRhiSmokeLaunchOperation(plan.Kind) || !IsRhiSmokeTestEnabled(plan))
		{
			return;
		}

		AddEnvironment(plan, "SPARKLE_SMOKE_VALIDATE_RHI", "1");
		AddEnvironment(plan, "SPARKLE_SMOKE_FRAME_LIMIT", GetRhiSmokeFrameLimitText(plan.Request));
		if (!plan.Request.SmokeBackend.empty())
		{
			AddEnvironment(plan, "SPARKLE_RHI_BACKEND", plan.Request.SmokeBackend);
		}
		if (!plan.Request.SmokeViewMode.empty())
		{
			AddEnvironment(plan, "SPARKLE_SMOKE_VIEW_MODE_NAME", plan.Request.SmokeViewMode);
			AddEnvironment(plan, "SPARKLE_SMOKE_VIEW_MODE", plan.Request.SmokeViewMode);
		}
		if (!plan.Request.SmokeCapturePath.empty())
		{
			AddEnvironment(plan, "SPARKLE_SMOKE_SCENE_COLOR_CAPTURE", plan.Request.SmokeCapturePath);
		}
		if (plan.Request.SmokeSkipLevelSwitching || HasRhiSmokeScenarioMatrix(plan))
		{
			AddEnvironment(plan, "SPARKLE_SMOKE_SKIP_LEVEL_SWITCHING", "1");
		}
	}

	std::vector<std::string> GetRhiSmokeLaunchPlannedEffects(const LaunchOperationPlan& plan)
	{
		if (!IsRhiSmokeLaunchOperation(plan.Kind) || !IsRhiSmokeTestEnabled(plan))
		{
			return {};
		}

		std::vector<std::string> effects = {
		    "Enable graphics smoke validation for " + GetRhiSmokeFrameLimitText(plan.Request) + " frames.",
		    "Run smoke suites: " + GetRhiSmokeSuiteSummary(plan) + "."};
		if (plan.Request.SmokeRunRayTracingParity)
		{
			effects.push_back("Capture backend/PTLAS parity evidence and validate cross-backend image comparisons.");
		}
		if (plan.Request.SmokeRunPtlasBenchmark)
		{
			effects.push_back("Capture PTLAS timing and metadata evidence and write benchmark summary diagnostics.");
		}
		if (!HasRhiSmokeScenarioMatrix(plan))
		{
			effects.push_back("Run a focused single-launch smoke pass using the selected target and launch parameters.");
		}
		if (!plan.Request.SmokeViewMode.empty())
		{
			effects.push_back("Force render view mode " + plan.Request.SmokeViewMode + " during smoke validation.");
		}
		if (!plan.Request.SmokeCapturePath.empty())
		{
			effects.push_back("Write scene-color smoke capture to " + plan.Request.SmokeCapturePath + ".");
		}
		return effects;
	}
}
