#include "SparkleLauncher/LaunchOperations.h"

#include "LaunchOperationProcessRequests.h"
#include "SparkleLauncher/LauncherPaths.h"
#include "SparkleLauncher/ToolResolver.h"

#include <algorithm>
#include <optional>
#include <sstream>
#include <system_error>
#include <utility>

namespace SparkleLauncher
{
	static void AddReadiness(LaunchOperationPlan& plan, std::string message)
	{
		plan.ReadinessMessages.push_back(std::move(message));
	}

	static void AddPlannedEffect(LaunchOperationPlan& plan, std::string message)
	{
		plan.PlannedEffects.push_back(std::move(message));
	}

	static std::string ResolveLaunchProfile(LaunchOperationKind kind, const LaunchOperationRequest& request)
	{
		return kind == LaunchOperationKind::RunEditor ? request.EditorProfile : request.RuntimeProfile;
	}

	static std::optional<BuildProfile> ResolveProfileForLaunch(LaunchOperationKind kind, std::string_view profileName)
	{
		const std::optional<BuildProfile> profile = FindBuildProfile(profileName);
		if (!profile.has_value())
		{
			return std::nullopt;
		}

		const BuildProfileTarget expectedTarget = kind == LaunchOperationKind::RunEditor ? BuildProfileTarget::Editor : BuildProfileTarget::Game;
		return profile->Target == expectedTarget ? profile : std::nullopt;
	}

	static void PopulateLaunchStep(LaunchOperationPlan& plan)
	{
		if (!plan.CanRun)
		{
			return;
		}

		for (const LaunchOperationProcessStep& processStep : BuildLaunchProcessStepsForPlan(plan))
		{
			LaunchOperationStep step;
			step.Id = processStep.Id;
			step.DisplayName = processStep.DisplayName;
			step.DisplayCommandLine = BuildDisplayCommandLine(processStep.Request.ExecutablePath, processStep.Request.Arguments);
			step.LogPath = processStep.Request.LogPath;
			plan.Steps.push_back(std::move(step));
		}
	}

	std::string ToString(LaunchOperationKind kind)
	{
		switch (kind)
		{
		case LaunchOperationKind::RunEditor:
			return "RunEditor";
		case LaunchOperationKind::RunRuntime:
			return "RunRuntime";
		}

		return "Unknown";
	}

	const std::vector<LaunchOperationDefinition>& GetLaunchOperationDefinitions()
	{
		static const std::vector<LaunchOperationDefinition> definitions = {
		    {LaunchOperationKind::RunEditor, "project.launch.editor", "Launch", "Run Editor", "Run the selected project's editor executable from its project directory."},
		    {LaunchOperationKind::RunRuntime, "project.launch.runtime", "Launch", "Run Runtime", "Run the selected project's runtime executable from its project directory."},
		};
		return definitions;
	}

	std::optional<LaunchOperationDefinition> FindLaunchOperationDefinition(std::string_view operationId)
	{
		const std::vector<LaunchOperationDefinition>& definitions = GetLaunchOperationDefinitions();
		const auto found = std::find_if(definitions.begin(), definitions.end(), [operationId](const LaunchOperationDefinition& definition) {
			return definition.Id == operationId;
		});
		return found == definitions.end() ? std::nullopt : std::optional<LaunchOperationDefinition>(*found);
	}

	LaunchOperationPlan PlanLaunchOperation(std::string_view operationId, const LaunchOperationRequest& request)
	{
		LaunchOperationPlan plan;
		const std::optional<LaunchOperationDefinition> definition = FindLaunchOperationDefinition(operationId);
		if (!definition.has_value())
		{
			plan.Operation = MakeOperationRecord(std::string(operationId), "Unknown launch operation");
			plan.Operation.FailureSummary = "Unknown launch operation id.";
			AddReadiness(plan, plan.Operation.FailureSummary);
			return plan;
		}

		plan.Kind = definition->Kind;
		plan.RepositoryRoot = request.RepositoryRoot;
		plan.Request = request;
		plan.Profile = ResolveLaunchProfile(plan.Kind, request);
		plan.Operation = MakeOperationRecord(definition->Id, definition->DisplayName);
		plan.Operation.Inputs.push_back({"project", request.ProjectId});
		plan.Operation.Inputs.push_back({"profile", plan.Profile});
		plan.Operation.LogPath = GetLauncherOperationLogPath(request.RepositoryRoot, definition->Id, "Latest.txt");

		const std::optional<BuildProfile> profile = ResolveProfileForLaunch(plan.Kind, plan.Profile);
		if (!profile.has_value())
		{
			AddReadiness(plan, "Launch profile does not match the requested launch target: " + plan.Profile);
			return plan;
		}

		plan.TargetName = BuildProjectTargetName(request.ProjectId, *profile);
		plan.ExecutablePath = ResolveSparkleToolPath(request.RepositoryRoot, plan.Profile, plan.TargetName);
		plan.WorkingDirectory = request.RepositoryRoot / "Projects" / request.ProjectId;

		std::error_code errorCode;
		const bool executableExists = std::filesystem::exists(plan.ExecutablePath, errorCode);
		errorCode.clear();
		const bool projectMarkerExists = std::filesystem::exists(plan.WorkingDirectory / ".sparkle-project", errorCode);
		AddReadiness(plan, executableExists ? "Launch executable exists." : "Launch executable is missing; compile the target first: " + plan.TargetName);
		AddReadiness(plan, projectMarkerExists ? "Project working directory is valid." : "Project working directory is missing or is not a Sparkle project: " + plan.WorkingDirectory.string());
		AddPlannedEffect(plan, "Launch " + plan.ExecutablePath.string() + " with working directory " + plan.WorkingDirectory.string() + ".");
		plan.CanRun = executableExists && projectMarkerExists;
		PopulateLaunchStep(plan);

		std::ostringstream dryRun;
		dryRun << "Dry-run plan for " << definition->DisplayName << ":";
		for (const LaunchOperationStep& step : plan.Steps)
		{
			dryRun << "\n  " << step.DisplayName << ": " << step.DisplayCommandLine;
			dryRun << "\n    Working directory: " << plan.WorkingDirectory.string();
			if (!step.LogPath.empty())
			{
				dryRun << "\n    Log: " << step.LogPath.string();
			}
		}
		if (plan.Steps.empty())
		{
			dryRun << "\n  No process step available until readiness issues are resolved.";
		}
		plan.Operation.DryRunText = dryRun.str();
		return plan;
	}
}