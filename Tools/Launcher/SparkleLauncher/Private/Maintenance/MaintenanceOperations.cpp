#include "SparkleLauncher/MaintenanceOperations.h"

#include "MaintenanceOperationProcessRequests.h"

#include <algorithm>
#include <optional>
#include <sstream>
#include <system_error>
#include <utility>

namespace SparkleLauncher
{
	static void AddReadiness(MaintenanceOperationPlan& plan, std::string message)
	{
		plan.ReadinessMessages.push_back(std::move(message));
	}

	static void AddPlannedEffect(MaintenanceOperationPlan& plan, std::string message)
	{
		plan.PlannedEffects.push_back(std::move(message));
	}

	static void CountCleanTarget(MaintenanceCleanTarget& target)
	{
		std::error_code errorCode;
		target.Exists = std::filesystem::exists(target.Path, errorCode);
		if (!target.Exists || errorCode)
		{
			return;
		}

		if (std::filesystem::is_regular_file(target.Path, errorCode))
		{
			target.FileCount = 1;
			target.ByteCount = std::filesystem::file_size(target.Path, errorCode);
			if (errorCode)
			{
				target.ByteCount = 0;
			}
			return;
		}

		if (!std::filesystem::is_directory(target.Path, errorCode))
		{
			return;
		}

		std::filesystem::recursive_directory_iterator iterator(
		    target.Path,
		    std::filesystem::directory_options::skip_permission_denied,
		    errorCode);
		const std::filesystem::recursive_directory_iterator end;
		while (iterator != end)
		{
			const std::filesystem::directory_entry entry = *iterator;
			if (entry.is_directory(errorCode))
			{
				++target.DirectoryCount;
			}
			else if (entry.is_regular_file(errorCode))
			{
				++target.FileCount;
				target.ByteCount += entry.file_size(errorCode);
			}
			errorCode.clear();
			iterator.increment(errorCode);
			errorCode.clear();
		}
	}

	static void AddCleanTarget(MaintenanceOperationPlan& plan, std::string displayName, std::filesystem::path path, std::string detail)
	{
		MaintenanceCleanTarget target;
		target.DisplayName = std::move(displayName);
		target.Path = std::move(path);
		target.Detail = std::move(detail);
		CountCleanTarget(target);
		plan.CleanTargets.push_back(std::move(target));
	}

	static std::string CleanTargetDisplayName(std::string_view processDisplayName)
	{
		static constexpr std::string_view cleanPrefix = "Clean ";
		return std::string(
		    processDisplayName.starts_with(cleanPrefix) ? processDisplayName.substr(cleanPrefix.size()) : processDisplayName);
	}

	static std::string CleanTargetDetail(const MaintenanceOperationProcessStep& step)
	{
		if (!step.PreviewDetail.empty())
		{
			return step.PreviewDetail;
		}

		switch (step.CleanBehavior)
		{
			case MaintenanceCleanBehavior::RemoveDirectoryContentsPreservingPath:
				return "Directory contents are removed except the preserved path: " + step.PreservedPath.string();
			case MaintenanceCleanBehavior::RemoveRootGeneratedFiles:
				return "Only recognized root CMake and IDE generated files are removed.";
			case MaintenanceCleanBehavior::RemovePath:
				return "The exact generated or user-local path selected by this clean scope is removed.";
		}
		return "Generated output selected by this clean scope.";
	}

	static void PopulateCleanTargets(MaintenanceOperationPlan& plan)
	{
		for (const MaintenanceOperationProcessStep& step : BuildMaintenanceCleanSteps(plan))
		{
			AddCleanTarget(plan, CleanTargetDisplayName(step.DisplayName), step.DestructivePath, CleanTargetDetail(step));
		}
	}

	static std::string FormatCleanTargetStats(const MaintenanceCleanTarget& target)
	{
		if (!target.Exists)
		{
			return "not present";
		}

		return std::to_string(target.FileCount) + " files, " + std::to_string(target.DirectoryCount) + " directories, "
		    + std::to_string(target.ByteCount) + " bytes";
	}

	static OperationDestructiveScope ToOperationDestructiveScope(CleanScope scope)
	{
		switch (scope)
		{
			case CleanScope::CookedOutputs:
				return OperationDestructiveScope::CookedOutputs;
			case CleanScope::BuildTree:
				return OperationDestructiveScope::BuildTree;
			case CleanScope::ArtifactOutputs:
				return OperationDestructiveScope::ArtifactOutputs;
			case CleanScope::WorkspaceState:
				return OperationDestructiveScope::WorkspaceState;
			case CleanScope::ThirdPartyDependencyCache:
				return OperationDestructiveScope::DependencyCache;
			case CleanScope::Logs:
				return OperationDestructiveScope::Logs;
			case CleanScope::PristineGeneratedWorkspace:
				return OperationDestructiveScope::PristineGeneratedWorkspace;
		}

		return OperationDestructiveScope::None;
	}

	static void PopulatePlanSteps(MaintenanceOperationPlan& plan)
	{
		if (!plan.CanRun)
		{
			return;
		}

		const std::vector<MaintenanceOperationProcessStep> processSteps = BuildMaintenanceProcessStepsForPlan(plan);
		for (const MaintenanceOperationProcessStep& processStep : processSteps)
		{
			MaintenanceOperationStep step;
			step.Id = processStep.Id;
			step.DisplayName = processStep.DisplayName;
			step.Destructive = processStep.DeletesGeneratedOutput;
			step.DestructivePath = processStep.DestructivePath;
			step.DisplayCommandLine = "Delete " + processStep.DestructivePath.string();
			plan.Steps.push_back(std::move(step));
		}
	}

	std::string ToString(MaintenanceOperationKind kind)
	{
		switch (kind)
		{
			case MaintenanceOperationKind::CleanWorkspace:
				return "CleanWorkspace";
		}

		return "Unknown";
	}

	std::string ToString(CleanScope scope)
	{
		return std::string(CleanScopeId(scope));
	}

	std::string_view CleanScopeId(CleanScope scope) noexcept
	{
		switch (scope)
		{
			case CleanScope::CookedOutputs:
				return "cooked";
			case CleanScope::BuildTree:
				return "build-tree";
			case CleanScope::ArtifactOutputs:
				return "artifacts";
			case CleanScope::WorkspaceState:
				return "workspace-state";
			case CleanScope::ThirdPartyDependencyCache:
				return "deps";
			case CleanScope::Logs:
				return "logs";
			case CleanScope::PristineGeneratedWorkspace:
				return "clean-all";
		}

		return "unknown";
	}

	bool CleanScopeRequiresContent(CleanScope scope) noexcept
	{
		switch (scope)
		{
			case CleanScope::CookedOutputs:
			case CleanScope::BuildTree:
			case CleanScope::WorkspaceState:
			case CleanScope::Logs:
			case CleanScope::PristineGeneratedWorkspace:
				return true;
			case CleanScope::ArtifactOutputs:
			case CleanScope::ThirdPartyDependencyCache:
				return false;
		}
		return false;
	}

	bool TryParseCleanScope(std::string_view text, CleanScope& outScope) noexcept
	{
		static constexpr CleanScope scopes[] = {
		    CleanScope::CookedOutputs,
		    CleanScope::BuildTree,
		    CleanScope::ArtifactOutputs,
		    CleanScope::WorkspaceState,
		    CleanScope::ThirdPartyDependencyCache,
		    CleanScope::Logs,
		    CleanScope::PristineGeneratedWorkspace};
		for (const CleanScope scope : scopes)
		{
			if (CleanScopeId(scope) == text)
			{
				outScope = scope;
				return true;
			}
		}
		return false;
	}

	const std::vector<MaintenanceOperationDefinition>& GetMaintenanceOperationDefinitions()
	{
		static const std::vector<MaintenanceOperationDefinition> definitions = {
		    {MaintenanceOperationKind::CleanWorkspace,
		        "workspace.clean",
		        "Clean",
		        "Clean Workspace",
		        "Remove generated files for the selected confirmed scope."},
		};
		return definitions;
	}

	std::optional<MaintenanceOperationDefinition> FindMaintenanceOperationDefinition(std::string_view operationId)
	{
		const std::vector<MaintenanceOperationDefinition>& definitions = GetMaintenanceOperationDefinitions();
		const auto found = std::find_if(
		    definitions.begin(),
		    definitions.end(),
		    [operationId](const MaintenanceOperationDefinition& definition) { return definition.Id == operationId; });
		return found == definitions.end() ? std::nullopt : std::optional<MaintenanceOperationDefinition>(*found);
	}

	MaintenanceOperationPlan PlanMaintenanceOperation(std::string_view operationId, const MaintenanceOperationRequest& request)
	{
		MaintenanceOperationPlan plan;
		const std::optional<MaintenanceOperationDefinition> definition = FindMaintenanceOperationDefinition(operationId);
		if (!definition.has_value())
		{
			plan.Operation = MakeOperationRecord(std::string(operationId), "Unknown maintenance operation");
			SetOperationFailure(
			    plan.Operation,
			    OperationProblemKind::Planning,
			    "Unknown maintenance operation id.",
			    "Choose a registered Clean operation, then retry.");
			AddReadiness(plan, plan.Operation.Failure->Summary);
			return plan;
		}

		plan.Kind = definition->Kind;
		plan.RepositoryRoot = request.RepositoryRoot;
		plan.Request = request;
		plan.Operation = MakeOperationRecord(definition->Id, definition->DisplayName);
		plan.Operation.Inputs.push_back({"content", request.ContentId});
		plan.Operation.Inputs.push_back({"editorProfile", request.EditorProfile});
		for (const MaintenanceCleanPathSpec& target : request.RequestedCleanTargets)
		{
			plan.Operation.Inputs.push_back({"cleanTarget", target.Path.string()});
		}
		for (const CleanScope scope : ResolveRequestedCleanScopes(request))
		{
			plan.Operation.Inputs.push_back({"cleanScope", ToString(scope)});
		}
		plan.Toolchain = DetectBuildToolchain(request.RepositoryRoot, WorkspaceIde::VisualStudio);
		plan.Freshness = CheckBuildFilesFreshness(request.RepositoryRoot, plan.Toolchain);

		switch (plan.Kind)
		{
			case MaintenanceOperationKind::CleanWorkspace:
			{
				const std::vector<CleanScope> requestedCleanScopes = ResolveRequestedCleanScopes(request);
				const bool missingContent = request.ContentId.empty() && request.RequestedCleanTargets.empty()
				    && std::any_of(requestedCleanScopes.begin(), requestedCleanScopes.end(), CleanScopeRequiresContent);
				if (missingContent)
				{
					SetOperationFailure(
					    plan.Operation,
					    OperationProblemKind::Prerequisite,
					    "The selected clean scope requires a content project.",
					    "Select a content project or remove project-owned outputs from the clean request, then retry.");
					AddReadiness(plan, plan.Operation.Failure->Summary);
					break;
				}

				PopulateCleanTargets(plan);
				plan.Operation.DestructiveScope = request.RequestedCleanTargets.empty() && requestedCleanScopes.size() == 1
				    ? ToOperationDestructiveScope(requestedCleanScopes.front())
				    : OperationDestructiveScope::None;
				plan.Operation.RequiresConfirmation = true;
				AddReadiness(
				    plan,
				    request.DestructiveActionConfirmed ? "Clean scope was confirmed." : "Clean scope requires explicit confirmation.");
				for (const MaintenanceCleanTarget& target : plan.CleanTargets)
				{
					AddPlannedEffect(
					    plan,
					    target.DisplayName + ": " + target.Path.string() + " (" + target.Detail + "; " + FormatCleanTargetStats(target)
					        + ")");
				}
				plan.CanRun = request.DestructiveActionConfirmed;
				break;
			}
		}

		PopulatePlanSteps(plan);

		std::ostringstream dryRun;
		dryRun << "Dry-run plan for " << definition->DisplayName << ":";
		if (!plan.Operation.LogPath.empty())
		{
			dryRun << "\n  Latest log: " << plan.Operation.LogPath.string();
		}
		if (plan.Operation.RequiresConfirmation)
		{
			dryRun << "\n  Confirmation required for clean scope: " << ToString(plan.Request.RequestedCleanScope);
		}
		for (const MaintenanceOperationStep& step : plan.Steps)
		{
			dryRun << "\n  " << step.DisplayName << ": " << step.DisplayCommandLine;
			if (!step.LogPath.empty())
			{
				dryRun << "\n    Log: " << step.LogPath.string();
			}
			if (step.Destructive)
			{
				dryRun << "\n    Scope: " << step.DestructivePath.string();
			}
		}
		if (plan.Steps.empty())
		{
			dryRun << "\n  No command step available until readiness issues are resolved.";
		}
		plan.Operation.DryRunText = dryRun.str();
		return plan;
	}
}
