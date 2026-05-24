#include "SparkleLauncher/BuildWorkspaceOperations.h"

#include "BuildWorkspaceProcessRequests.h"
#include "Core/Public/Strings/StringUtils.h"
#include "SparkleLauncher/BuildProfileCatalog.h"
#include "SparkleLauncher/LauncherPaths.h"

#include <algorithm>
#include <optional>
#include <sstream>
#include <utility>

namespace SparkleLauncher
{
	static void AddReadiness(BuildWorkspaceOperationPlan& plan, std::string message)
	{
		plan.ReadinessMessages.push_back(std::move(message));
	}

	static void AddPlannedEffect(BuildWorkspaceOperationPlan& plan, std::string message)
	{
		plan.PlannedEffects.push_back(std::move(message));
	}

	static std::vector<std::string> ResolveProjectTargets(std::string_view projectId, std::string_view profileName)
	{
		const std::optional<BuildProfile> profile = FindBuildProfile(profileName);
		if (!profile.has_value())
		{
			return {};
		}
		return {BuildProjectTargetName(projectId, *profile)};
	}

	static void AddConfigureStep(BuildWorkspaceOperationPlan& plan)
	{
		AddPlannedEffect(plan, "Run CMake configure with generator '" + plan.Toolchain.Generator + "'.");
	}

	static void AddBuildStep(BuildWorkspaceOperationPlan& plan, std::string_view profileName, const std::vector<std::string>& targets)
	{
		std::vector<std::string_view> targetViews;
		for (const std::string& target : targets)
		{
			targetViews.push_back(target);
		}
		AddPlannedEffect(plan, "Build " + Strings::Join(targetViews, ", ") + " for " + std::string(profileName) + ".");
	}

	static void PopulatePlanSteps(BuildWorkspaceOperationPlan& plan, const BuildWorkspaceOperationRequest& request)
	{
		if (!plan.Toolchain.RequiredToolsAvailable)
		{
			AddReadiness(plan, "Required build tools are missing.");
			return;
		}

		const bool needsConfigure = request.ForceConfigure || !plan.Freshness.Current;
		switch (plan.Kind)
		{
		case BuildWorkspaceOperationKind::CheckToolchain:
			AddPlannedEffect(plan, "Report native CMake, Visual Studio/MSBuild, Windows SDK, Git, and clang-format status.");
			plan.CanRun = true;
			return;
		case BuildWorkspaceOperationKind::SetupWorkspace:
			if (needsConfigure)
			{
				AddConfigureStep(plan);
			}
			else
			{
				AddPlannedEffect(plan, "Build files are already current; setup has no command step.");
			}
			plan.CanRun = true;
			return;
		case BuildWorkspaceOperationKind::GenerateSolution:
			AddConfigureStep(plan);
			plan.CanRun = true;
			return;
		case BuildWorkspaceOperationKind::OpenSolution:
			if (needsConfigure)
			{
				AddConfigureStep(plan);
			}
			AddPlannedEffect(plan, "Open solution: " + plan.Freshness.SolutionPath.string());
			plan.CanRun = true;
			return;
		case BuildWorkspaceOperationKind::CompileEditor:
		{
			if (needsConfigure)
			{
				AddConfigureStep(plan);
			}
			std::vector<std::string> targets = request.SelectedTargets.empty() ? ResolveProjectTargets(request.ProjectId, request.EditorProfile) : request.SelectedTargets;
			if (targets.empty())
			{
				AddReadiness(plan, "No editor build target could be resolved.");
				return;
			}
			AddBuildStep(plan, request.EditorProfile, targets);
			plan.CanRun = true;
			return;
		}
		case BuildWorkspaceOperationKind::CompileRuntime:
		{
			if (needsConfigure)
			{
				AddConfigureStep(plan);
			}
			std::vector<std::string> targets = request.SelectedTargets.empty() ? ResolveProjectTargets(request.ProjectId, request.RuntimeProfile) : request.SelectedTargets;
			if (targets.empty())
			{
				AddReadiness(plan, "No runtime build target could be resolved.");
				return;
			}
			AddBuildStep(plan, request.RuntimeProfile, targets);
			plan.CanRun = true;
			return;
		}
		case BuildWorkspaceOperationKind::BuildCookTools:
			if (needsConfigure)
			{
				AddConfigureStep(plan);
			}
			AddBuildStep(plan, request.EditorProfile, {"AssetCooker", "TextureCooker", "ShaderCompiler"});
			plan.CanRun = true;
			return;
		}
	}

	const std::vector<BuildWorkspaceOperationDefinition>& GetBuildWorkspaceOperationDefinitions()
	{
		static const std::vector<BuildWorkspaceOperationDefinition> definitions = {
		    {BuildWorkspaceOperationKind::SetupWorkspace, "workspace.setup", "Setup", "Setup Workspace", "Validate required tools and refresh the Visual Studio solution."},
		    {BuildWorkspaceOperationKind::GenerateSolution, "workspace.generate-solution", "Setup", "Regenerate Solution", "Refresh the Visual Studio solution and project files."},
		    {BuildWorkspaceOperationKind::OpenSolution, "workspace.open-solution", "Run", "Open Solution", "Open the generated Visual Studio solution."},
		    {BuildWorkspaceOperationKind::CheckToolchain, "toolchain.check", "Setup", "Check Toolchain", "Inspect CMake, Visual Studio/MSBuild, Windows SDK, Git, and clang-format."},
		    {BuildWorkspaceOperationKind::CompileEditor, "project.build.editor", "Build", "Build Editor", "Build the selected project's editor target."},
		    {BuildWorkspaceOperationKind::CompileRuntime, "project.build.runtime", "Build", "Build Runtime", "Build the selected project's runtime target."},
		    {BuildWorkspaceOperationKind::BuildCookTools, "cook.tools.prepare", "Build", "Build Cook Tools", "Build the tools required by cooking workflows."},
		};
		return definitions;
	}

	std::optional<BuildWorkspaceOperationDefinition> FindBuildWorkspaceOperationDefinition(std::string_view operationId)
	{
		const std::vector<BuildWorkspaceOperationDefinition>& definitions = GetBuildWorkspaceOperationDefinitions();
		const auto found = std::find_if(definitions.begin(), definitions.end(), [operationId](const BuildWorkspaceOperationDefinition& definition) {
			return definition.Id == operationId;
		});
		return found == definitions.end() ? std::nullopt : std::optional<BuildWorkspaceOperationDefinition>(*found);
	}

	BuildWorkspaceOperationPlan PlanBuildWorkspaceOperation(std::string_view operationId, const BuildWorkspaceOperationRequest& request)
	{
		BuildWorkspaceOperationPlan plan;
		const std::optional<BuildWorkspaceOperationDefinition> definition = FindBuildWorkspaceOperationDefinition(operationId);
		if (!definition.has_value())
		{
			plan.Operation = MakeOperationRecord(std::string(operationId), "Unknown build operation");
			plan.Operation.FailureSummary = "Unknown build/workspace operation id.";
			AddReadiness(plan, plan.Operation.FailureSummary);
			return plan;
		}

		plan.Kind = definition->Kind;
		plan.RepositoryRoot = request.RepositoryRoot;
		plan.Operation = MakeOperationRecord(definition->Id, definition->DisplayName);
		plan.Operation.Inputs.push_back({"project", request.ProjectId});
		plan.Operation.Inputs.push_back({"editorProfile", request.EditorProfile});
		plan.Operation.Inputs.push_back({"runtimeProfile", request.RuntimeProfile});
		plan.Operation.LogPath = GetLauncherOperationLogPath(request.RepositoryRoot, definition->Id, "Latest.txt");
		plan.Request = request;
		plan.Toolchain = DetectBuildToolchain(request.RepositoryRoot);
		plan.Freshness = CheckBuildFilesFreshness(request.RepositoryRoot, plan.Toolchain);

		AddReadiness(plan, plan.Toolchain.RequiredToolsAvailable ? "Required toolchain is available." : "Required toolchain is incomplete.");
		AddReadiness(plan, plan.Freshness.Summary);
		PopulatePlanSteps(plan, request);
		if (plan.CanRun)
		{
			const std::vector<BuildWorkspaceProcessStep> processSteps = BuildProcessStepsForPlan(plan);
			for (const BuildWorkspaceProcessStep& processStep : processSteps)
			{
				BuildWorkspaceOperationStep step;
				step.Id = processStep.Id;
				step.DisplayName = processStep.DisplayName;
				step.DisplayCommandLine = BuildDisplayCommandLine(processStep.Request.ExecutablePath, processStep.Request.Arguments);
				step.LogPath = processStep.Request.LogPath;
				step.UpdatesBuildFilesFreshness = processStep.UpdatesBuildFilesFreshness;
				plan.Steps.push_back(std::move(step));
			}
		}

		std::ostringstream dryRun;
		dryRun << "Dry-run plan for " << definition->DisplayName << ":";
		for (const BuildWorkspaceOperationStep& step : plan.Steps)
		{
			dryRun << "\n  " << step.DisplayName << ": " << step.DisplayCommandLine;
		}
		if (plan.Steps.empty())
		{
			dryRun << "\n  No command step required.";
		}
		plan.Operation.DryRunText = dryRun.str();
		return plan;
	}
}