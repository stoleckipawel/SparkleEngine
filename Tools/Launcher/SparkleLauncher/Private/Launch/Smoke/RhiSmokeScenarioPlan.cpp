#include "Smoke/RhiSmokeScenarioPlan.h"

#include "Smoke/RhiSmokeLaunchOperations.h"
#include "Smoke/RhiSmokeProcessRequestBuilder.h"
#include "Smoke/RhiSmokeTestCatalog.h"

#include <utility>

namespace SparkleLauncher
{
	namespace
	{
		ProcessRequest BuildScenarioRequest(
		    const LaunchOperationPlan& plan,
		    RhiSmokeSuite suite,
		    const RhiSmokeScenarioCase& scenarioCase,
		    const RhiSmokeScenarioViewMode& viewMode)
		{
			const RhiSmokeSuiteDefinition& suiteDefinition = GetRhiSmokeSuiteDefinition(suite);
			ProcessRequest request = BuildRhiSmokeBaseProcessRequest(plan);
			if (!std::string(scenarioCase.Backend).empty())
			{
				request.Arguments[1] = scenarioCase.Backend;
				AddOrReplaceRhiSmokeEnvironment(request, "SPARKLE_RHI_BACKEND", scenarioCase.Backend);
			}
			else if (!plan.Request.SmokeBackend.empty())
			{
				request.Arguments[1] = plan.Request.SmokeBackend;
			}
			AddRhiSmokeCVar(
			    request,
			    std::string("r.RayTracing.PreferPartitionedTlas=") + (scenarioCase.PreferPartitionedTlas ? "true" : "false"));
			AddRhiSmokeCVar(
			    request,
			    std::string("r.RayTracing.Ptlas.OperationWriterPath=") + scenarioCase.PtlasOperationWriterPath);

			const std::string capturePath = GetRhiSmokeArtifactPath(plan, suite, scenarioCase, viewMode, ".bmp").string();
			const std::string metadataPath = GetRhiSmokeArtifactPath(plan, suite, scenarioCase, viewMode, ".json").string();
			const std::string timingPath = GetRhiSmokeArtifactPath(plan, suite, scenarioCase, viewMode, ".timing.csv").string();

			AddOrReplaceRhiSmokeEnvironment(request, "SPARKLE_SMOKE_VIEW_MODE_NAME", viewMode.Name);
			AddOrReplaceRhiSmokeEnvironment(request, "SPARKLE_SMOKE_SCENE_COLOR_CAPTURE", capturePath);
			AddOrReplaceRhiSmokeEnvironment(request, "SPARKLE_SMOKE_METADATA_PATH", metadataPath);
			AddOrReplaceRhiSmokeEnvironment(request, "SPARKLE_SMOKE_TIMING_CSV", timingPath);
			AddOrReplaceRhiSmokeEnvironment(request, "SPARKLE_SMOKE_SCENE_COLOR_CAPTURE_FRAME", std::to_string(suiteDefinition.CaptureFrame));
			AddOrReplaceRhiSmokeEnvironment(request, "SPARKLE_SMOKE_FRAME_LIMIT", GetRhiSmokeFrameLimitText(plan.Request));
			AddOrReplaceRhiSmokeEnvironment(request, "SPARKLE_SMOKE_SKIP_LEVEL_SWITCHING", "1");
			if (suiteDefinition.MotionEndFrame > suiteDefinition.MotionStartFrame)
			{
				AddOrReplaceRhiSmokeEnvironment(request, "SPARKLE_SMOKE_CAMERA_MOTION", "1");
				AddOrReplaceRhiSmokeEnvironment(request, "SPARKLE_SMOKE_CAMERA_MOTION_REQUIRES_RT", "1");
				AddOrReplaceRhiSmokeEnvironment(request, "SPARKLE_SMOKE_CAMERA_MOTION_START_FRAME", std::to_string(suiteDefinition.MotionStartFrame));
				AddOrReplaceRhiSmokeEnvironment(request, "SPARKLE_SMOKE_CAMERA_MOTION_END_FRAME", std::to_string(suiteDefinition.MotionEndFrame));
				AddOrReplaceRhiSmokeEnvironment(request, "SPARKLE_SMOKE_CAMERA_MOTION_YAW_DEGREES", std::to_string(suiteDefinition.MotionYawDegrees));
				AddOrReplaceRhiSmokeEnvironment(request, "SPARKLE_SMOKE_CAMERA_MOTION_PITCH_DEGREES", std::to_string(suiteDefinition.MotionPitchDegrees));
			}
			if (suite == RhiSmokeSuite::DiagnosticCaptures)
			{
				AddOrReplaceRhiSmokeEnvironment(request, "SPARKLE_SMOKE_CAPTURE_PURPOSE", viewMode.Purpose);
				AddOrReplaceRhiSmokeEnvironment(request, "SPARKLE_SMOKE_CAPTURE_LABEL", scenarioCase.CaptureLabel);
			}
			request.LogPath = GetRhiSmokeArtifactDirectory(plan, suite) / scenarioCase.Name / (std::string(viewMode.Name) + ".log");
			return request;
		}
	}

	std::vector<LaunchOperationProcessStep> BuildRhiSmokeScenarioProcessSteps(const LaunchOperationPlan& plan)
	{
		std::vector<LaunchOperationProcessStep> steps;
		for (const RhiSmokeSuite suite : GetEnabledRhiSmokeSuites(plan))
		{
			for (const RhiSmokeScenarioCase& scenarioCase : GetRhiSmokeCases(suite))
			{
				for (const RhiSmokeScenarioViewMode& viewMode : GetRhiSmokeViewModes(suite))
				{
					LaunchOperationProcessStep step;
					step.Id = std::string("rhi-smoke-") + GetRhiSmokeSuiteDefinition(suite).Id + "-" + scenarioCase.Name + "-" + viewMode.Name;
					step.DisplayName = std::string(GetRhiSmokeSuiteDefinition(suite).DisplayName) + ": " + scenarioCase.Name + " " + viewMode.Name;
					step.Request = BuildScenarioRequest(plan, suite, scenarioCase, viewMode);
					steps.push_back(std::move(step));
				}
			}
		}
		return steps;
	}
}
