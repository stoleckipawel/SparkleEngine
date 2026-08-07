#include "SparkleLauncher/LevelRunOperations.h"

#include "LevelRunOperationProcessRequests.h"
#include "Core/Public/FileSystemUtils.h"
#include "SparkleLauncher/LauncherPaths.h"
#include "SparkleLauncher/ToolResolver.h"

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

	static bool DirectoryHasRegularFiles(const std::filesystem::path& directory)
	{
		std::error_code errorCode;
		if (!std::filesystem::is_directory(directory, errorCode))
		{
			return false;
		}

		std::filesystem::recursive_directory_iterator iterator(
		    directory,
		    std::filesystem::directory_options::skip_permission_denied,
		    errorCode);
		const std::filesystem::recursive_directory_iterator end;
		while (iterator != end)
		{
			const std::filesystem::directory_entry entry = *iterator;
			if (entry.is_regular_file(errorCode))
			{
				return true;
			}
			errorCode.clear();
			iterator.increment(errorCode);
			errorCode.clear();
		}
		return false;
	}

	static bool CookedAssetScopeHasFiles(
	    const std::filesystem::path& repositoryRoot,
	    std::string_view projectId,
	    std::string_view relativeDirectory)
	{
		const std::string relativeScope(relativeDirectory);
		return DirectoryHasRegularFiles(GetCookedProjectDirectory(repositoryRoot, projectId) / relativeScope)
		    || DirectoryHasRegularFiles(GetSharedCookedProjectDirectory(repositoryRoot) / relativeScope);
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

	const std::vector<LevelRunOperationDefinition>& GetLevelRunOperationDefinitions()
	{
		static const std::vector<LevelRunOperationDefinition> definitions = {
		    {"levels.run", "Levels", "Run Level", "Run a catalog level after its runtime prerequisites are ready."},
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
			plan.Operation.FailureSummary = "Unknown level run operation id.";
			AddReadiness(plan, plan.Operation.FailureSummary);
			return plan;
		}

		plan.RepositoryRoot = request.RepositoryRoot;
		plan.Request = request;
		plan.Operation = MakeOperationRecord(definition->Id, definition->DisplayName);
		plan.Operation.Inputs = {
		    {"content", plan.Request.ContentId},
		    {"profile", plan.Request.RuntimeProfile},
		    {"level", plan.Request.LevelId},
		    {"graphicsApi", plan.Request.GraphicsApi}};
		plan.Operation.LogPath = GetLauncherOperationLogPath(plan.Request.RepositoryRoot, definition->Id, "Latest.txt");
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

		const std::optional<BuildProfile> profile = FindBuildProfile(plan.Request.RuntimeProfile);
		if (!profile.has_value() || profile->Target != BuildProfileTarget::Game)
		{
			AddReadiness(plan, "Runtime profile does not match a game target: " + plan.Request.RuntimeProfile);
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
		plan.ExecutablePath = FirstExistingOrPreferred({
		    GetProjectTargetArtifactDirectory(plan.Request.RepositoryRoot, plan.Request.ContentId, "runtime", plan.Request.RuntimeProfile)
		        / fileName,
		    ResolveSparkleToolPath(plan.Request.RepositoryRoot, plan.Request.RuntimeProfile, plan.TargetName),
		});
		plan.WorkingDirectory = plan.Request.RepositoryRoot / "Projects" / plan.Request.ContentId;
		AddEnvironment(plan, "SPARKLE_STARTUP_LEVEL", plan.Request.LevelId);

		std::error_code errorCode;
		plan.Readiness.ExecutableReady = std::filesystem::is_regular_file(plan.ExecutablePath, errorCode);
		errorCode.clear();
		plan.Readiness.ContentDirectoryReady =
		    std::filesystem::exists(plan.WorkingDirectory / std::string(Filesystem::kProjectMarker), errorCode);
		plan.Readiness.CookedMeshesReady = CookedAssetScopeHasFiles(plan.Request.RepositoryRoot, plan.Request.ContentId, "Meshes");
		plan.Readiness.CookedTexturesReady = CookedAssetScopeHasFiles(plan.Request.RepositoryRoot, plan.Request.ContentId, "Textures");
		plan.Readiness.CookedShadersReady = CookedAssetScopeHasFiles(plan.Request.RepositoryRoot, plan.Request.ContentId, "Shaders");

		AddReadiness(
		    plan,
		    plan.Readiness.ExecutableReady ? "Runtime executable is ready."
		                                   : "Runtime executable is missing; compile " + plan.TargetName + " first.");
		AddReadiness(
		    plan,
		    plan.Readiness.ContentDirectoryReady ? "Content working directory is valid."
		                                         : "Content working directory is missing or invalid: " + plan.WorkingDirectory.string());
		AddReadiness(
		    plan,
		    plan.Readiness.CookedMeshesReady ? "Cooked scenes and meshes are ready." : "Cooked scenes and meshes are missing.");
		AddReadiness(plan, plan.Readiness.CookedTexturesReady ? "Cooked textures are ready." : "Cooked textures are missing.");
		AddReadiness(plan, plan.Readiness.CookedShadersReady ? "Cooked shaders are ready." : "Cooked shaders are missing.");
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
