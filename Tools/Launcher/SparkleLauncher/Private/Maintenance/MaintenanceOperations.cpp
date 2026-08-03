#include "SparkleLauncher/MaintenanceOperations.h"

#include "MaintenanceOperationProcessRequests.h"
#include "SparkleLauncher/LauncherPaths.h"

#include <algorithm>
#include <cstdint>
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

	static void AddContentGeneratedTargets(MaintenanceOperationPlan& plan, bool includeBuild, bool includeLogs, bool includeState)
	{
		const std::filesystem::path contentPath = plan.RepositoryRoot / "Projects" / plan.Request.ContentId;
		if (includeBuild)
		{
			AddCleanTarget(plan, "Content build tree", contentPath / "build", "Generated content build files.");
		}
		if (includeLogs)
		{
			AddCleanTarget(plan, "Content logs", contentPath / "logs", "Content diagnostic logs.");
		}
		if (includeState)
		{
			AddCleanTarget(plan, "Content ImGui state", contentPath / "imgui.ini", "Content UI state.");
		}
	}

	static void PopulateCleanTargetsForScope(MaintenanceOperationPlan& plan, CleanScope scope)
	{
		switch (scope)
		{
			case CleanScope::CookedOutputs:
				AddCleanTarget(
				    plan,
				    "Cooked content",
				    GetCookedProjectDirectory(plan.RepositoryRoot, plan.Request.ContentId),
				    "Generated cooked assets for this workspace.");
				return;
			case CleanScope::BuildTree:
				AddCleanTarget(
				    plan,
				    "Build tree contents",
				    GetBuildDirectory(plan.RepositoryRoot),
				    "Contents are removed except build/_deps.");
				AddCleanTarget(
				    plan,
				    "Root generated CMake/VS files",
				    plan.RepositoryRoot,
				    "Root *.sln, *.slnx, *.vcxproj, CMakeCache.txt, cmake_install.cmake, Makefile, and CMakeFiles.");
				AddContentGeneratedTargets(plan, true, false, false);
				return;
			case CleanScope::ArtifactOutputs:
				AddCleanTarget(
				    plan,
				    "Generated artifacts",
				    GetArtifactDirectory(plan.RepositoryRoot),
				    "Generated runnable artifacts, diagnostics, libraries, symbols, and cooked outputs.");
				return;
			case CleanScope::WorkspaceState:
				AddCleanTarget(plan, "Visual Studio workspace state", plan.RepositoryRoot / ".vs", ".vs directory.");
				AddCleanTarget(plan, "VS Code workspace state", plan.RepositoryRoot / ".vscode", ".vscode directory.");
				AddCleanTarget(plan, "Rider workspace state", plan.RepositoryRoot / ".idea", ".idea directory.");
				AddCleanTarget(plan, "Root ImGui state", plan.RepositoryRoot / "imgui.ini", "Root imgui.ini.");
				AddContentGeneratedTargets(plan, false, false, true);
				return;
			case CleanScope::ShaderCache:
				AddCleanTarget(
				    plan,
				    "Shader cache",
				    GetBuildDirectory(plan.RepositoryRoot) / "Cache" / "Shaders",
				    "Local shader cache, recook signal, and transient shader outputs.");
				return;
			case CleanScope::ThirdPartyDependencyCache:
				AddCleanTarget(
				    plan,
				    "Third-party dependency cache",
				    GetBuildDirectory(plan.RepositoryRoot) / "_deps",
				    "FetchContent dependency cache; configure will re-download dependencies.");
				return;
			case CleanScope::Logs:
				AddCleanTarget(plan, "Repository logs", plan.RepositoryRoot / "logs", "Root structured logs.");
				AddCleanTarget(
				    plan,
				    "Launcher logs",
				    GetLauncherStatePaths(plan.RepositoryRoot).LogsDirectory,
				    "Per-repository launcher logs stored in the user-local launcher state directory.");
				AddContentGeneratedTargets(plan, false, true, false);
				return;
			case CleanScope::PristineGeneratedWorkspace:
				AddCleanTarget(
				    plan,
				    "Build tree",
				    GetBuildDirectory(plan.RepositoryRoot),
				    "Full build tree including dependency cache and private build-system outputs.");
				AddCleanTarget(
				    plan,
				    "Development artifacts",
				    GetArtifactDirectory(plan.RepositoryRoot),
				    "Generated runnable artifacts, diagnostics, libraries, symbols, and cooked outputs.");
				AddCleanTarget(plan, "Visual Studio workspace state", plan.RepositoryRoot / ".vs", ".vs directory.");
				AddCleanTarget(plan, "VS Code workspace state", plan.RepositoryRoot / ".vscode", ".vscode directory.");
				AddCleanTarget(plan, "Rider workspace state", plan.RepositoryRoot / ".idea", ".idea directory.");
				AddCleanTarget(plan, "Repository logs", plan.RepositoryRoot / "logs", "Root structured logs.");
				AddCleanTarget(
				    plan,
				    "Launcher state",
				    GetLauncherStatePaths(plan.RepositoryRoot).RootDirectory,
				    "Per-repository launcher logs, activity history, and cached workflow state.");
				AddCleanTarget(plan, "Root ImGui state", plan.RepositoryRoot / "imgui.ini", "Root imgui.ini.");
				AddCleanTarget(
				    plan,
				    "Root generated CMake/VS files",
				    plan.RepositoryRoot,
				    "Root *.sln, *.slnx, *.vcxproj, CMakeCache.txt, cmake_install.cmake, Makefile, and CMakeFiles.");
				AddContentGeneratedTargets(plan, true, true, true);
				return;
		}
	}

	static std::vector<CleanScope> ResolveRequestedCleanScopes(const MaintenanceOperationRequest& request)
	{
		std::vector<CleanScope> scopes = request.RequestedCleanScopes;
		if (scopes.empty())
		{
			scopes.push_back(request.RequestedCleanScope);
		}

		std::vector<CleanScope> uniqueScopes;
		for (const CleanScope scope : scopes)
		{
			if (std::find(uniqueScopes.begin(), uniqueScopes.end(), scope) == uniqueScopes.end())
			{
				uniqueScopes.push_back(scope);
			}
		}
		return uniqueScopes;
	}

	static void PopulateCleanTargets(MaintenanceOperationPlan& plan)
	{
		if (!plan.Request.RequestedCleanTargets.empty())
		{
			for (const MaintenanceCleanPathSpec& target : plan.Request.RequestedCleanTargets)
			{
				AddCleanTarget(plan, target.DisplayName, target.Path, target.Detail);
			}
			return;
		}

		for (const CleanScope scope : ResolveRequestedCleanScopes(plan.Request))
		{
			PopulateCleanTargetsForScope(plan, scope);
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
			case CleanScope::ShaderCache:
				return OperationDestructiveScope::ShaderCache;
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
		switch (scope)
		{
			case CleanScope::CookedOutputs:
				return "cooked";
			case CleanScope::BuildTree:
				return "build-tree";
			case CleanScope::ArtifactOutputs:
				return "artifact-outputs";
			case CleanScope::WorkspaceState:
				return "workspace-state";
			case CleanScope::ShaderCache:
				return "shader-cache";
			case CleanScope::ThirdPartyDependencyCache:
				return "deps";
			case CleanScope::Logs:
				return "logs";
			case CleanScope::PristineGeneratedWorkspace:
				return "clean-all";
		}

		return "unknown";
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
			plan.Operation.FailureSummary = "Unknown maintenance operation id.";
			AddReadiness(plan, plan.Operation.FailureSummary);
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
