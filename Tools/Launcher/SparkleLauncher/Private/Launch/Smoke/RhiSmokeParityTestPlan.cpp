#include "Smoke/RhiSmokeParityTestPlan.h"

#include "Smoke/RhiSmokeLaunchOperations.h"
#include "Smoke/RhiSmokeTestCatalog.h"

#include <algorithm>
#include <string>
#include <utility>

namespace SparkleLauncher::RhiSmokeParityStepEnvironment
{
	void AddOrReplace(ProcessRequest& request, std::string name, std::string value)
	{
		const auto found = std::find_if(
		    request.Environment.begin(),
		    request.Environment.end(),
		    [&name](const EnvironmentOverride& overrideValue)
		    {
			    return overrideValue.Name == name;
		    });
		if (found != request.Environment.end())
		{
			found->Value = std::move(value);
			return;
		}

		request.Environment.push_back(EnvironmentOverride{std::move(name), std::move(value)});
	}
}

namespace SparkleLauncher::RhiSmokeParityProcessRequest
{
	void AddCVar(ProcessRequest& request, std::string cvar)
	{
		request.Arguments.push_back("--cvar");
		request.Arguments.push_back(std::move(cvar));
	}

	ProcessRequest BuildBaseRequest(const LaunchOperationPlan& plan)
	{
		ProcessRequest request;
		request.ExecutablePath = plan.ExecutablePath;
		request.WorkingDirectory = plan.WorkingDirectory;
		request.Environment = plan.Environment;
		request.Arguments.push_back("--graphics-api");
		request.Arguments.push_back("");
		AddCVar(request, "r.VSync=false");
		return request;
	}

	ProcessRequest BuildCaseViewRequest(
	    const LaunchOperationPlan& plan,
	    const RhiSmokeParityCase& parityCase,
	    const RhiSmokeParityViewMode& viewMode)
	{
		ProcessRequest request = BuildBaseRequest(plan);
		request.Arguments[1] = parityCase.Backend;
		AddCVar(
		    request,
		    std::string("r.RayTracing.PreferPartitionedTlas=") +
		        (parityCase.PreferPartitionedTlas ? "true" : "false"));

		RhiSmokeParityStepEnvironment::AddOrReplace(request, "SPARKLE_RHI_BACKEND", parityCase.Backend);
		RhiSmokeParityStepEnvironment::AddOrReplace(request, "SPARKLE_SMOKE_VIEW_MODE_NAME", viewMode.Name);
		RhiSmokeParityStepEnvironment::AddOrReplace(
		    request,
		    "SPARKLE_SMOKE_SCENE_COLOR_CAPTURE",
		    GetRhiSmokeParityArtifactPath(plan, parityCase, viewMode, ".bmp").string());
		RhiSmokeParityStepEnvironment::AddOrReplace(
		    request,
		    "SPARKLE_SMOKE_METADATA_PATH",
		    GetRhiSmokeParityArtifactPath(plan, parityCase, viewMode, ".json").string());
		RhiSmokeParityStepEnvironment::AddOrReplace(
		    request,
		    "SPARKLE_SMOKE_TIMING_CSV",
		    GetRhiSmokeParityArtifactPath(plan, parityCase, viewMode, ".timing.csv").string());
		RhiSmokeParityStepEnvironment::AddOrReplace(request, "SPARKLE_SMOKE_SCENE_COLOR_CAPTURE_FRAME", "50");
		RhiSmokeParityStepEnvironment::AddOrReplace(request, "SPARKLE_SMOKE_FRAME_LIMIT", GetRhiSmokeFrameLimitText(plan.Request));
		RhiSmokeParityStepEnvironment::AddOrReplace(request, "SPARKLE_SMOKE_CAMERA_MOTION", "1");
		RhiSmokeParityStepEnvironment::AddOrReplace(request, "SPARKLE_SMOKE_CAMERA_MOTION_REQUIRES_RT", "1");
		RhiSmokeParityStepEnvironment::AddOrReplace(request, "SPARKLE_SMOKE_CAMERA_MOTION_START_FRAME", "10");
		RhiSmokeParityStepEnvironment::AddOrReplace(request, "SPARKLE_SMOKE_CAMERA_MOTION_END_FRAME", "40");
		RhiSmokeParityStepEnvironment::AddOrReplace(request, "SPARKLE_SMOKE_CAMERA_MOTION_YAW_DEGREES", "12");
		RhiSmokeParityStepEnvironment::AddOrReplace(request, "SPARKLE_SMOKE_CAMERA_MOTION_PITCH_DEGREES", "0");
		request.LogPath = GetRhiSmokeParityArtifactDirectory(plan) / parityCase.Name / (std::string(viewMode.Name) + ".log");
		return request;
	}
}

namespace SparkleLauncher
{
	std::vector<LaunchOperationProcessStep> BuildRhiSmokeRayTracingParityProcessSteps(const LaunchOperationPlan& plan)
	{
		std::vector<LaunchOperationProcessStep> steps;
		for (const RhiSmokeParityCase& parityCase : GetRhiSmokeParityCases())
		{
			for (const RhiSmokeParityViewMode& viewMode : GetRhiSmokeParityViewModes())
			{
				LaunchOperationProcessStep step;
				step.Id = std::string("rhi-parity-") + parityCase.Name + "-" + viewMode.Name;
				step.DisplayName = std::string("Capture ") + parityCase.Name + " " + viewMode.Name;
				step.Request = RhiSmokeParityProcessRequest::BuildCaseViewRequest(plan, parityCase, viewMode);
				steps.push_back(std::move(step));
			}
		}
		return steps;
	}
}
