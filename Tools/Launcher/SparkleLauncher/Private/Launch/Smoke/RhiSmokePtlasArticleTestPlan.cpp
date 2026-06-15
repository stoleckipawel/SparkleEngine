#include "Smoke/RhiSmokePtlasArticleTestPlan.h"

#include "Smoke/RhiSmokeLaunchOperations.h"
#include "Smoke/RhiSmokeProcessRequestBuilder.h"
#include "Smoke/RhiSmokeTestCatalog.h"

#include <utility>

namespace SparkleLauncher::RhiSmokePtlasArticleProcessRequest
{
	ProcessRequest BuildCaseViewRequest(
	    const LaunchOperationPlan& plan,
	    const RhiSmokePtlasArticleCase& articleCase,
	    const RhiSmokePtlasArticleViewMode& viewMode)
	{
		ProcessRequest request = BuildRhiSmokeBaseProcessRequest(plan);
		request.Arguments[1] = articleCase.Backend;
		AddRhiSmokeCVar(
		    request,
		    std::string("r.RayTracing.PreferPartitionedTlas=") +
		        (articleCase.PreferPartitionedTlas ? "true" : "false"));
		AddRhiSmokeCVar(
		    request,
		    std::string("r.RayTracing.Ptlas.OperationWriterPath=") + articleCase.PtlasOperationWriterPath);

		AddOrReplaceRhiSmokeEnvironment(request, "SPARKLE_RHI_BACKEND", articleCase.Backend);
		AddOrReplaceRhiSmokeEnvironment(request, "SPARKLE_SMOKE_VIEW_MODE_NAME", viewMode.Name);
		AddOrReplaceRhiSmokeEnvironment(
		    request,
		    "SPARKLE_SMOKE_SCENE_COLOR_CAPTURE",
		    GetRhiSmokePtlasArticleArtifactPath(plan, articleCase, viewMode, ".bmp").string());
		AddOrReplaceRhiSmokeEnvironment(
		    request,
		    "SPARKLE_SMOKE_METADATA_PATH",
		    GetRhiSmokePtlasArticleArtifactPath(plan, articleCase, viewMode, ".json").string());
		AddOrReplaceRhiSmokeEnvironment(
		    request,
		    "SPARKLE_SMOKE_TIMING_CSV",
		    GetRhiSmokePtlasArticleArtifactPath(plan, articleCase, viewMode, ".timing.csv").string());
		AddOrReplaceRhiSmokeEnvironment(request, "SPARKLE_SMOKE_CAPTURE_PURPOSE", viewMode.Purpose);
		AddOrReplaceRhiSmokeEnvironment(request, "SPARKLE_SMOKE_CAPTURE_STORY_LABEL", articleCase.StoryLabel);
		AddOrReplaceRhiSmokeEnvironment(request, "SPARKLE_SMOKE_SCENE_COLOR_CAPTURE_FRAME", "80");
		AddOrReplaceRhiSmokeEnvironment(request, "SPARKLE_SMOKE_FRAME_LIMIT", GetRhiSmokeFrameLimitText(plan.Request));
		AddOrReplaceRhiSmokeEnvironment(request, "SPARKLE_SMOKE_SKIP_LEVEL_SWITCHING", "1");
		AddOrReplaceRhiSmokeEnvironment(request, "SPARKLE_SMOKE_CAMERA_MOTION", "1");
		AddOrReplaceRhiSmokeEnvironment(request, "SPARKLE_SMOKE_CAMERA_MOTION_REQUIRES_RT", "1");
		AddOrReplaceRhiSmokeEnvironment(request, "SPARKLE_SMOKE_CAMERA_MOTION_START_FRAME", "10");
		AddOrReplaceRhiSmokeEnvironment(request, "SPARKLE_SMOKE_CAMERA_MOTION_END_FRAME", "70");
		AddOrReplaceRhiSmokeEnvironment(request, "SPARKLE_SMOKE_CAMERA_MOTION_YAW_DEGREES", "24");
		AddOrReplaceRhiSmokeEnvironment(request, "SPARKLE_SMOKE_CAMERA_MOTION_PITCH_DEGREES", "0");
		request.LogPath = GetRhiSmokePtlasArticleArtifactDirectory(plan) / articleCase.Name / (std::string(viewMode.Name) + ".log");
		return request;
	}
}

namespace SparkleLauncher
{
	std::vector<LaunchOperationProcessStep> BuildRhiSmokePtlasArticleProcessSteps(const LaunchOperationPlan& plan)
	{
		std::vector<LaunchOperationProcessStep> steps;
		for (const RhiSmokePtlasArticleCase& articleCase : GetRhiSmokePtlasArticleCases())
		{
			for (const RhiSmokePtlasArticleViewMode& viewMode : GetRhiSmokePtlasArticleViewModes())
			{
				LaunchOperationProcessStep step;
				step.Id = std::string("rhi-ptlas-article-") + articleCase.Name + "-" + viewMode.Name;
				step.DisplayName = std::string("Article capture ") + articleCase.Name + " " + viewMode.Name;
				step.Request = RhiSmokePtlasArticleProcessRequest::BuildCaseViewRequest(plan, articleCase, viewMode);
				steps.push_back(std::move(step));
			}
		}
		return steps;
	}
}
