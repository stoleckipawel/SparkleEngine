#include "Smoke/RhiSmokePtlasBenchmarkTestPlan.h"

#include "Smoke/RhiSmokeLaunchOperations.h"
#include "Smoke/RhiSmokeProcessRequestBuilder.h"
#include "Smoke/RhiSmokeTestCatalog.h"

#include <utility>

namespace SparkleLauncher::RhiSmokePtlasBenchmarkProcessRequest
{
	ProcessRequest BuildCaseViewRequest(
	    const LaunchOperationPlan& plan,
	    const RhiSmokePtlasBenchmarkCase& benchmarkCase,
	    const RhiSmokePtlasBenchmarkViewMode& viewMode)
	{
		ProcessRequest request = BuildRhiSmokeBaseProcessRequest(plan);
		request.Arguments[1] = benchmarkCase.Backend;
		AddRhiSmokeCVar(
		    request,
		    std::string("r.RayTracing.PreferPartitionedTlas=") +
		        (benchmarkCase.PreferPartitionedTlas ? "true" : "false"));
		AddRhiSmokeCVar(
		    request,
		    std::string("r.RayTracing.Ptlas.OperationWriterPath=") + benchmarkCase.PtlasOperationWriterPath);

		AddOrReplaceRhiSmokeEnvironment(request, "SPARKLE_RHI_BACKEND", benchmarkCase.Backend);
		AddOrReplaceRhiSmokeEnvironment(request, "SPARKLE_SMOKE_VIEW_MODE_NAME", viewMode.Name);
		AddOrReplaceRhiSmokeEnvironment(
		    request,
		    "SPARKLE_SMOKE_SCENE_COLOR_CAPTURE",
		    GetRhiSmokePtlasBenchmarkArtifactPath(plan, benchmarkCase, viewMode, ".bmp").string());
		AddOrReplaceRhiSmokeEnvironment(
		    request,
		    "SPARKLE_SMOKE_METADATA_PATH",
		    GetRhiSmokePtlasBenchmarkArtifactPath(plan, benchmarkCase, viewMode, ".json").string());
		AddOrReplaceRhiSmokeEnvironment(
		    request,
		    "SPARKLE_SMOKE_TIMING_CSV",
		    GetRhiSmokePtlasBenchmarkArtifactPath(plan, benchmarkCase, viewMode, ".timing.csv").string());
		AddOrReplaceRhiSmokeEnvironment(request, "SPARKLE_SMOKE_SCENE_COLOR_CAPTURE_FRAME", "80");
		AddOrReplaceRhiSmokeEnvironment(request, "SPARKLE_SMOKE_FRAME_LIMIT", GetRhiSmokeFrameLimitText(plan.Request));
		AddOrReplaceRhiSmokeEnvironment(request, "SPARKLE_SMOKE_SKIP_LEVEL_SWITCHING", "1");
		AddOrReplaceRhiSmokeEnvironment(request, "SPARKLE_SMOKE_CAMERA_MOTION", "1");
		AddOrReplaceRhiSmokeEnvironment(request, "SPARKLE_SMOKE_CAMERA_MOTION_REQUIRES_RT", "1");
		AddOrReplaceRhiSmokeEnvironment(request, "SPARKLE_SMOKE_CAMERA_MOTION_START_FRAME", "10");
		AddOrReplaceRhiSmokeEnvironment(request, "SPARKLE_SMOKE_CAMERA_MOTION_END_FRAME", "70");
		AddOrReplaceRhiSmokeEnvironment(request, "SPARKLE_SMOKE_CAMERA_MOTION_YAW_DEGREES", "24");
		AddOrReplaceRhiSmokeEnvironment(request, "SPARKLE_SMOKE_CAMERA_MOTION_PITCH_DEGREES", "0");
		request.LogPath = GetRhiSmokePtlasBenchmarkArtifactDirectory(plan) / benchmarkCase.Name / (std::string(viewMode.Name) + ".log");
		return request;
	}
}

namespace SparkleLauncher
{
	std::vector<LaunchOperationProcessStep> BuildRhiSmokePtlasBenchmarkProcessSteps(const LaunchOperationPlan& plan)
	{
		std::vector<LaunchOperationProcessStep> steps;
		for (const RhiSmokePtlasBenchmarkCase& benchmarkCase : GetRhiSmokePtlasBenchmarkCases())
		{
			for (const RhiSmokePtlasBenchmarkViewMode& viewMode : GetRhiSmokePtlasBenchmarkViewModes())
			{
				LaunchOperationProcessStep step;
				step.Id = std::string("rhi-ptlas-benchmark-") + benchmarkCase.Name + "-" + viewMode.Name;
				step.DisplayName = std::string("Benchmark ") + benchmarkCase.Name + " " + viewMode.Name;
				step.Request = RhiSmokePtlasBenchmarkProcessRequest::BuildCaseViewRequest(plan, benchmarkCase, viewMode);
				steps.push_back(std::move(step));
			}
		}
		return steps;
	}
}
