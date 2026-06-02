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

	static bool RequireCurrentWorkspace(BuildWorkspaceOperationPlan& plan)
	{
		if (plan.Freshness.Current)
		{
			return true;
		}

		AddReadiness(plan, "Solution/workspace files are not current. Run Regenerate Solution first.");
		return false;
	}

	static bool IsPreferredIdeAvailable(const BuildToolchainStatus& toolchain, WorkspaceIde ide)
	{
		switch (ide)
		{
		case WorkspaceIde::VisualStudio:
			return !toolchain.VswherePath.empty();
		case WorkspaceIde::Rider:
			return !toolchain.RiderPath.empty();
		}

		return false;
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

	static std::vector<std::string> GetEnabledCookToolTargets()
	{
		std::vector<std::string> targets;
#if SPARKLE_ENABLE_CONTENT_PIPELINE
		targets.push_back("AssetCooker");
		targets.push_back("TextureCooker");
#endif
#if SPARKLE_ENABLE_SHADER_COMPILER
		targets.push_back("ShaderCompiler");
#endif
		return targets;
	}

	static void AddConfigureStep(BuildWorkspaceOperationPlan& plan)
	{
		AddPlannedEffect(plan, "Run CMake configure with generator '" + plan.Toolchain.Generator + "' for " + DisplayName(plan.Request.PreferredIde) + ".");
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
			AddReadiness(plan, "Required Visual Studio Windows build tools are missing or unsupported. ClangCL remains supported when selected as the CMake toolset.");
			return;
		}

		const bool needsConfigure = request.ForceConfigure || !plan.Freshness.Current;
		switch (plan.Kind)
		{
		case BuildWorkspaceOperationKind::CheckToolchain:
			AddPlannedEffect(plan, "Report required host dependencies for the supported Windows workflow: Visual Studio, an MSVC-compatible Qt 6 kit, CMake, Windows SDK, Git, and optional ClangCL or IDE integrations.");
			plan.CanRun = true;
			return;
		case BuildWorkspaceOperationKind::SetupWorkspace:
			if (needsConfigure)
			{
				AddConfigureStep(plan);
			}
			else
			{
				AddPlannedEffect(plan, "Workspace configure is current; no command step required.");
			}
			plan.CanRun = true;
			return;
		case BuildWorkspaceOperationKind::GenerateSolution:
			AddConfigureStep(plan);
			plan.CanRun = true;
			return;
		case BuildWorkspaceOperationKind::OpenSolution:
			if (!IsPreferredIdeAvailable(plan.Toolchain, plan.Request.PreferredIde))
			{
				AddReadiness(plan, DisplayName(plan.Request.PreferredIde) + " is not available on this machine.");
				return;
			}
			if (!RequireCurrentWorkspace(plan))
			{
				return;
			}
			if (plan.Request.PreferredIde == WorkspaceIde::Rider)
			{
				AddPlannedEffect(plan, "Open Rider at repository root: " + plan.RepositoryRoot.string());
			}
			else
			{
				AddPlannedEffect(plan, "Open Visual Studio solution: " + plan.Freshness.SolutionPath.string());
			}
			plan.CanRun = true;
			return;
		case BuildWorkspaceOperationKind::BuildAll:
			if (!RequireCurrentWorkspace(plan))
			{
				return;
			}
			AddBuildStep(plan, request.EditorProfile, {"SparkleLauncher"});
			{
				std::vector<std::string> editorTargets = ResolveProjectTargets(request.ProjectId, request.EditorProfile);
				if (editorTargets.empty())
				{
					AddReadiness(plan, "No editor build target could be resolved.");
					return;
				}
				AddBuildStep(plan, request.EditorProfile, editorTargets);
			}
			{
				std::vector<std::string> runtimeTargets = ResolveProjectTargets(request.ProjectId, request.RuntimeProfile);
				if (runtimeTargets.empty())
				{
					AddReadiness(plan, "No runtime build target could be resolved.");
					return;
				}
				AddBuildStep(plan, request.RuntimeProfile, runtimeTargets);
			}
			{
				const std::vector<std::string> cookToolTargets = GetEnabledCookToolTargets();
				if (!cookToolTargets.empty())
				{
					AddBuildStep(plan, request.EditorProfile, cookToolTargets);
					AddPlannedEffect(plan, "Build launcher, selected project editor/runtime, and enabled cook tools in one pass.");
				}
				else
				{
					AddPlannedEffect(plan, "Build launcher plus selected project editor/runtime targets. Optional cook tools are disabled in this workspace.");
				}
			}
			plan.CanRun = true;
			return;
		case BuildWorkspaceOperationKind::CompileLauncher:
			if (!RequireCurrentWorkspace(plan))
			{
				return;
			}
			AddBuildStep(plan, request.EditorProfile, {"SparkleLauncher"});
			AddPlannedEffect(plan, "Rebuild the launcher executable and deployed runtime files.");
			plan.CanRun = true;
			return;
		case BuildWorkspaceOperationKind::CompileEditor:
		{
			if (!RequireCurrentWorkspace(plan))
			{
				return;
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
			if (!RequireCurrentWorkspace(plan))
			{
				return;
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
			if (!RequireCurrentWorkspace(plan))
			{
				return;
			}
			{
				const std::vector<std::string> cookToolTargets = GetEnabledCookToolTargets();
				if (cookToolTargets.empty())
				{
					AddReadiness(plan, "No optional cook-tool targets are enabled in this workspace configuration.");
					return;
				}
				AddBuildStep(plan, request.EditorProfile, cookToolTargets);
			}
			plan.CanRun = true;
			return;
		}
	}

	const std::vector<BuildWorkspaceOperationDefinition>& GetBuildWorkspaceOperationDefinitions()
	{
		static const std::vector<BuildWorkspaceOperationDefinition> definitions = {
		    {BuildWorkspaceOperationKind::SetupWorkspace, "workspace.setup", "Setup", "Sync Third Parties", "Run the third-party dependency sync/configure step for the workspace."},
		    {BuildWorkspaceOperationKind::GenerateSolution, "workspace.generate-solution", "Setup", "Regenerate Solution", "Force-refresh generated solution and workspace files for the selected IDE."},
		    {BuildWorkspaceOperationKind::OpenSolution, "workspace.open-solution", "Run", "Open IDE", "Open the selected IDE for this workspace."},
		    {BuildWorkspaceOperationKind::CheckToolchain, "toolchain.check", "Setup", "Check Dependencies", "Inspect required host dependencies without changing workspace files."},
		    {BuildWorkspaceOperationKind::BuildAll, "workspace.build-all", "Build", "Build All", "Build launcher, project editor/runtime targets, and any enabled optional cook tools."},
		    {BuildWorkspaceOperationKind::CompileLauncher, "launcher.build.self", "Build", "Build Launcher", "Build the SparkleLauncher target so the launcher can restart into the new binary."},
		    {BuildWorkspaceOperationKind::CompileEditor, "project.build.editor", "Build", "Build Editor", "Build the selected project's editor target."},
		    {BuildWorkspaceOperationKind::CompileRuntime, "project.build.runtime", "Build", "Build Runtime", "Build the selected project's runtime target."},
		    {BuildWorkspaceOperationKind::BuildCookTools, "cook.tools.prepare", "Build", "Build Cook Tools", "Build the enabled optional tools required by cooking workflows."},
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
		plan.Operation.Inputs.push_back({"workspaceIde", WorkspaceIdeCommandLineValue(request.PreferredIde)});
		plan.Operation.LogPath = GetLauncherOperationLogPath(request.RepositoryRoot, definition->Id, "Latest.txt");
		plan.Request = request;
		plan.Toolchain = DetectBuildToolchain(request.RepositoryRoot, request.PreferredIde);
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
