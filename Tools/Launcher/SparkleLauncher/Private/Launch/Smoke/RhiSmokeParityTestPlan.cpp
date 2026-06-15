#include "Smoke/RhiSmokeParityTestPlan.h"

#include "Smoke/RhiSmokeLaunchOperations.h"
#include "Smoke/RhiSmokeProcessRequestBuilder.h"
#include "Smoke/RhiSmokeTestCatalog.h"

#include <utility>

namespace SparkleLauncher::RhiSmokeParityProcessRequest
{
	ProcessRequest BuildCaseViewRequest(
	    const LaunchOperationPlan& plan,
	    const RhiSmokeParityCase& parityCase,
	    const RhiSmokeParityViewMode& viewMode)
	{
		ProcessRequest request = BuildRhiSmokeBaseProcessRequest(plan);
		request.Arguments[1] = parityCase.Backend;
		AddRhiSmokeCVar(
		    request,
		    std::string("r.RayTracing.PreferPartitionedTlas=") +
		        (parityCase.PreferPartitionedTlas ? "true" : "false"));
		AddRhiSmokeCVar(
		    request,
		    std::string("r.RayTracing.Ptlas.OperationWriterPath=") + parityCase.PtlasOperationWriterPath);

		AddOrReplaceRhiSmokeEnvironment(request, "SPARKLE_RHI_BACKEND", parityCase.Backend);
		AddOrReplaceRhiSmokeEnvironment(request, "SPARKLE_SMOKE_VIEW_MODE_NAME", viewMode.Name);
		AddOrReplaceRhiSmokeEnvironment(
		    request,
		    "SPARKLE_SMOKE_SCENE_COLOR_CAPTURE",
		    GetRhiSmokeParityArtifactPath(plan, parityCase, viewMode, ".bmp").string());
		AddOrReplaceRhiSmokeEnvironment(
		    request,
		    "SPARKLE_SMOKE_METADATA_PATH",
		    GetRhiSmokeParityArtifactPath(plan, parityCase, viewMode, ".json").string());
		AddOrReplaceRhiSmokeEnvironment(
		    request,
		    "SPARKLE_SMOKE_TIMING_CSV",
		    GetRhiSmokeParityArtifactPath(plan, parityCase, viewMode, ".timing.csv").string());
		AddOrReplaceRhiSmokeEnvironment(request, "SPARKLE_SMOKE_SCENE_COLOR_CAPTURE_FRAME", "50");
		AddOrReplaceRhiSmokeEnvironment(request, "SPARKLE_SMOKE_FRAME_LIMIT", GetRhiSmokeFrameLimitText(plan.Request));
		AddOrReplaceRhiSmokeEnvironment(request, "SPARKLE_SMOKE_SKIP_LEVEL_SWITCHING", "1");
		AddOrReplaceRhiSmokeEnvironment(request, "SPARKLE_SMOKE_CAMERA_MOTION", "1");
		AddOrReplaceRhiSmokeEnvironment(request, "SPARKLE_SMOKE_CAMERA_MOTION_REQUIRES_RT", "1");
		AddOrReplaceRhiSmokeEnvironment(request, "SPARKLE_SMOKE_CAMERA_MOTION_START_FRAME", "10");
		AddOrReplaceRhiSmokeEnvironment(request, "SPARKLE_SMOKE_CAMERA_MOTION_END_FRAME", "40");
		AddOrReplaceRhiSmokeEnvironment(request, "SPARKLE_SMOKE_CAMERA_MOTION_YAW_DEGREES", "12");
		AddOrReplaceRhiSmokeEnvironment(request, "SPARKLE_SMOKE_CAMERA_MOTION_PITCH_DEGREES", "0");
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
