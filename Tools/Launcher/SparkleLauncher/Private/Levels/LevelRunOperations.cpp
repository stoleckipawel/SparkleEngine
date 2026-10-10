#include "SparkleLauncher/LevelRunOperations.h"

#include "CookedContentReadiness.h"
#include "LevelRunOperationProcessRequests.h"
#include "ExternalCaptureDiscovery.h"
#include "Core/Public/FileSystemUtils.h"
#include "LauncherStatePaths.h"
#include "SparkleLauncher/ToolResolver.h"
#include "Core/Public/Paths/WorkspaceOutputPaths.h"

#include <algorithm>
#include <optional>
#include <sstream>
#include <system_error>
#include <utility>

namespace SparkleLauncher
{
	static void AddReadiness(LevelRunOperationPlan& plan, std::string message)
	{
		plan.ReadinessMessages.push_back(std::move(message));
	}

	static void AddPlannedEffect(LevelRunOperationPlan& plan, std::string message)
	{
		plan.PlannedEffects.push_back(std::move(message));
	}

	static void AddEnvironment(LevelRunOperationPlan& plan, std::string name, std::string value)
	{
		Process::EnvironmentOverride overrideValue;
		overrideValue.Name = std::move(name);
		overrideValue.Value = std::move(value);
		plan.Environment.push_back(std::move(overrideValue));
	}

	static std::filesystem::path FirstExistingOrPreferred(const std::vector<std::filesystem::path>& candidates)
	{
		std::error_code errorCode;
		for (const std::filesystem::path& candidate : candidates)
		{
			if (std::filesystem::exists(candidate, errorCode) && std::filesystem::is_regular_file(candidate, errorCode))
			{
				return candidate;
			}
			errorCode.clear();
		}
		return candidates.empty() ? std::filesystem::path() : candidates.front();
	}

	static void PopulateRunStep(LevelRunOperationPlan& plan)
	{
		if (!plan.CanRun)
		{
			return;
		}
		for (const LevelRunOperationProcessStep& processStep : BuildLevelRunProcessStepsForPlan(plan))
		{
			LevelRunOperationStep step;
			step.Id = processStep.Id;
			step.DisplayName = processStep.DisplayName;
			step.DisplayCommandLine = BuildDisplayCommandLine(processStep.Request.ExecutablePath, processStep.Request.Arguments);
			step.LogPath = processStep.Request.LogPath;
			plan.Steps.push_back(std::move(step));
		}
	}

	std::string_view ToString(LevelRunMode mode) noexcept
	{
		switch (mode)
		{
			case LevelRunMode::Editor:
				return "editor";
			case LevelRunMode::Game:
				return "game";
		}

		return "unknown";
	}

	const std::vector<LevelRunOperationDefinition>& GetLevelRunOperationDefinitions()
	{
		static const std::vector<LevelRunOperationDefinition> definitions = {
		    {"levels.run", "Levels", "Run Level", "Open a catalog level in the selected product after its prerequisites are ready."},
		};
		return definitions;
	}

	std::optional<LevelRunOperationDefinition> FindLevelRunOperationDefinition(std::string_view operationId)
	{
		const std::vector<LevelRunOperationDefinition>& definitions = GetLevelRunOperationDefinitions();
		const auto found = std::find_if(
		    definitions.begin(),
		    definitions.end(),
		    [operationId](const LevelRunOperationDefinition& definition) { return definition.Id == operationId; });
		return found == definitions.end() ? std::nullopt : std::optional<LevelRunOperationDefinition>(*found);
	}

	LevelRunOperationPlan PlanLevelRunOperation(std::string_view operationId, const LevelRunOperationRequest& request)
	{
		LevelRunOperationPlan plan;
		const std::optional<LevelRunOperationDefinition> definition = FindLevelRunOperationDefinition(operationId);
		if (!definition.has_value())
		{
			plan.Operation = MakeOperationRecord(std::string(operationId), "Unknown level run operation");
			SetOperationFailure(
			    plan.Operation,
			    OperationProblemKind::Planning,
			    "Unknown level run operation id.",
			    "Choose the registered Run Level operation, then retry.");
			AddReadiness(plan, plan.Operation.Failure->Summary);
			return plan;
		}

		plan.RepositoryRoot = request.RepositoryRoot;
		plan.Request = request;
		plan.Operation = MakeOperationRecord(definition->Id, definition->DisplayName);
		plan.Operation.Inputs = {
		    {"content", plan.Request.ContentId},
		    {"runMode", std::string(ToString(plan.Request.RunMode))},
		    {"profile", plan.Request.ProductProfile},
		    {"level", plan.Request.LevelId},
		    {"graphicsApi", plan.Request.GraphicsApi},
		    {"captureProvider", std::string(ExternalCaptureProviderToString(plan.Request.CaptureProvider))}};
		plan.Operation.LogPath = ResolveLauncherOperationLogPath(plan.Request.RepositoryRoot, definition->Id, "Latest.txt");
		if (plan.Request.LevelId.empty())
		{
			AddReadiness(plan, "A catalog level id is required.");
			return plan;
		}
		if (plan.Request.GraphicsApi != "d3d12" && plan.Request.GraphicsApi != "vulkan")
		{
			AddReadiness(plan, "Unknown graphics API: " + plan.Request.GraphicsApi);
			return plan;
		}

		const auto capture =
		    InspectExternalCaptureProvider(plan.Request.CaptureProvider, plan.Request.GraphicsApi, plan.Request.ProductProfile);
		if (!capture.Available())
		{
			AddReadiness(plan, capture.Detail);
			return plan;
		}

		const BuildProfileTarget expectedTarget =
		    plan.Request.RunMode == LevelRunMode::Editor ? BuildProfileTarget::Editor : BuildProfileTarget::Game;
		const std::optional<BuildProfile> profile = FindBuildProfile(plan.Request.ProductProfile);
		if (!profile.has_value() || profile->Target != expectedTarget)
		{
			AddReadiness(
			    plan,
			    "Profile does not match the selected " + std::string(ToString(plan.Request.RunMode))
			        + " run mode: " + plan.Request.ProductProfile);
			return plan;
		}

		plan.TargetName = BuildProjectTargetName(plan.Request.ContentId, *profile);
		std::filesystem::path fileName(plan.TargetName);
#if defined(_WIN32)
		if (fileName.extension().empty())
		{
			fileName += ".exe";
		}
#endif
		const Filesystem::WorkspaceOutputPaths outputs = Filesystem::ResolveWorkspaceOutputPaths(plan.Request.RepositoryRoot);
		plan.ExecutablePath = FirstExistingOrPreferred({
		    outputs.ProjectTargetOutputs(
		               plan.Request.ContentId,
		               plan.Request.RunMode == LevelRunMode::Editor ? "editor" : "runtime",
		               plan.Request.ProductProfile)
		            .BinaryDirectory
		        / fileName,
		    ResolveSparkleToolPath(plan.Request.RepositoryRoot, plan.Request.ProductProfile, plan.TargetName),
		});
		plan.WorkingDirectory = plan.Request.RepositoryRoot / "Projects" / plan.Request.ContentId;
		AddEnvironment(plan, "SPARKLE_STARTUP_LEVEL", plan.Request.LevelId);

		std::error_code errorCode;
		plan.Readiness.ExecutableReady = std::filesystem::is_regular_file(plan.ExecutablePath, errorCode);
		errorCode.clear();
		plan.Readiness.ContentDirectoryReady =
		    std::filesystem::exists(plan.WorkingDirectory / std::string(Filesystem::kProjectMarker), errorCode);
		const CookedContentReadiness cookedContent = InspectCookedContentReadiness(plan.Request.RepositoryRoot, plan.Request.ContentId);
		plan.Readiness.CookedMeshesReady = cookedContent.MeshesReady;
		plan.Readiness.CookedTexturesReady = cookedContent.TexturesReady;
		plan.Readiness.CookedShadersReady = cookedContent.Shaders == CookedShaderPublicationState::Ready;

		AddReadiness(
		    plan,
		    plan.Readiness.ExecutableReady ? "Selected product executable is ready."
		                                   : "Selected product executable is missing; compile " + plan.TargetName + " first.");
		AddReadiness(
		    plan,
		    plan.Readiness.ContentDirectoryReady ? "Content working directory is valid."
		                                         : "Content working directory is missing or invalid: " + plan.WorkingDirectory.string());
		AddReadiness(
		    plan,
		    plan.Readiness.CookedMeshesReady ? "Cooked scenes and meshes are ready." : "Cooked scenes and meshes are missing.");
		AddReadiness(plan, plan.Readiness.CookedTexturesReady ? "Cooked textures are ready." : "Cooked textures are missing.");
		switch (cookedContent.Shaders)
		{
			case CookedShaderPublicationState::Ready:
				AddReadiness(plan, "Cooked shaders are ready.");
				break;
			case CookedShaderPublicationState::Invalid:
				AddReadiness(plan, "Cooked shader publication is incomplete or internally inconsistent.");
				break;
			case CookedShaderPublicationState::Missing:
				AddReadiness(plan, "The complete cooked shader generation is missing.");
				break;
		}
		AddPlannedEffect(
		    plan,
		    "Run level " + plan.Request.LevelId + " in " + plan.ExecutablePath.string() + " from " + plan.WorkingDirectory.string() + ".");
		AddPlannedEffect(plan, "Use graphics API: " + plan.Request.GraphicsApi + ".");

		plan.CanRun = plan.Readiness.ExecutableReady && plan.Readiness.ContentDirectoryReady && plan.Readiness.CookedMeshesReady
		    && plan.Readiness.CookedTexturesReady && plan.Readiness.CookedShadersReady;
		PopulateRunStep(plan);

		std::ostringstream dryRun;
		dryRun << "Dry-run plan for " << definition->DisplayName << ":";
		for (const LevelRunOperationStep& step : plan.Steps)
		{
			dryRun << "\n  " << step.DisplayName << ": " << step.DisplayCommandLine;
			dryRun << "\n    Working directory: " << plan.WorkingDirectory.string();
			dryRun << "\n    Env: SPARKLE_STARTUP_LEVEL=" << plan.Request.LevelId;
			if (!step.LogPath.empty())
			{
				dryRun << "\n    Log: " << step.LogPath.string();
			}
		}
		if (plan.Steps.empty())
		{
			dryRun << "\n  No command step is available until readiness issues are resolved.";
		}
		plan.Operation.DryRunText = dryRun.str();
		return plan;
	}
}
