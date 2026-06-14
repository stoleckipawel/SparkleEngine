#include "Smoke/RhiSmokeLaunchOperations.h"

#include <utility>

namespace SparkleLauncher
{
	bool IsRhiSmokeLaunchOperation(LaunchOperationKind kind)
	{
		return kind == LaunchOperationKind::RunProject || kind == LaunchOperationKind::RunRhiRayTracingParitySmoke;
	}

	bool IsRhiParitySmokeLaunchOperation(LaunchOperationKind kind)
	{
		return kind == LaunchOperationKind::RunRhiRayTracingParitySmoke;
	}

	bool IsRhiSmokeTestEnabled(const LaunchOperationPlan& plan) noexcept
	{
		return GetRhiSmokeTestCategory(plan) != RhiSmokeTestCategory::None;
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
		if (!IsRhiSmokeTestEnabled(plan))
		{
			return;
		}

		plan.Operation.Inputs.push_back({"smokeCategory", RhiSmokeTestCategoryToString(GetRhiSmokeTestCategory(plan))});
		plan.Operation.Inputs.push_back({"smokeBackend", plan.Request.SmokeBackend.empty() ? "default" : plan.Request.SmokeBackend});
		plan.Operation.Inputs.push_back({"smokeFrameLimit", GetRhiSmokeFrameLimitText(plan.Request)});
		plan.Operation.Inputs.push_back({"smokeViewMode", plan.Request.SmokeViewMode.empty() ? "default" : plan.Request.SmokeViewMode});
		plan.Operation.Inputs.push_back({"smokeCapturePath", plan.Request.SmokeCapturePath.empty() ? "disabled" : plan.Request.SmokeCapturePath});
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
		if (!IsRhiSmokeTestEnabled(plan))
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
		if (!plan.Request.SmokeViewMode.empty())
		{
			AddEnvironment(plan, "SPARKLE_SMOKE_VIEW_MODE_NAME", plan.Request.SmokeViewMode);
			AddEnvironment(plan, "SPARKLE_SMOKE_VIEW_MODE", plan.Request.SmokeViewMode);
		}
		if (!plan.Request.SmokeCapturePath.empty())
		{
			AddEnvironment(plan, "SPARKLE_SMOKE_SCENE_COLOR_CAPTURE", plan.Request.SmokeCapturePath);
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
		if (!IsRhiSmokeTestEnabled(plan))
		{
			return {};
		}

		std::vector<std::string> effects = {
		    std::string("Enable ") + RhiSmokeTestCategoryToString(GetRhiSmokeTestCategory(plan)) +
		    " graphics smoke validation for " + GetRhiSmokeFrameLimitText(plan.Request) + " frames."};
		if (GetRhiSmokeTestCategory(plan) == RhiSmokeTestCategory::RayTracingParity)
		{
			effects.push_back("Run D3D12/Vulkan classic TLAS and PTLAS parity captures with provider metadata and timing artifacts.");
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
