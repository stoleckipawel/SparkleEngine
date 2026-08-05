#include "SparkleLauncher/BuildWorkspaceOperations.h"

#include "NativeBuildOutputReset.h"
#include "BuildWorkspaceProcessRequests.h"
#include "HostToolInstaller.h"
#include "Core/Public/Diagnostics/Error.h"
#include "Core/Public/Strings/StringUtils.h"
#include "SparkleLauncher/ArtifactNaming.h"
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

		AddReadiness(plan, "Generated build files are not current. Run Generate Build Files first.");
		return false;
	}

	static bool RequireConfigurePrerequisites(BuildWorkspaceOperationPlan& plan)
	{
		if (plan.Toolchain.ConfigurePrerequisitesAvailable)
		{
			return true;
		}

		AddReadiness(plan, "Enabled workspace configure prerequisites are incomplete.");
		for (const ToolchainItemStatus& item : plan.Toolchain.Items)
		{
			if ((item.Id == "shader-compiler-sdk" || item.Id == "vulkan-sdk") && item.State != ToolchainItemState::Found
			    && !item.Detail.empty())
			{
				AddReadiness(plan, item.Detail);
			}
		}
		return false;
	}

	static bool RequireSyncedSourceDependencies(BuildWorkspaceOperationPlan& plan)
	{
		if (!HasIncompleteEnabledSourceDependencies(plan))
		{
			return true;
		}

		AddReadiness(plan, "Enabled repository dependencies are incomplete. Run Sync Code before local build workflows.");
		return false;
	}

	static bool IsPreferredIdeAvailable(const BuildToolchainStatus& toolchain, WorkspaceIde ide)
	{
		switch (ide)
		{
			case WorkspaceIde::VisualStudio:
				return !toolchain.VisualStudioIdePath.empty();
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
		AddPlannedEffect(
		    plan,
		    "Run CMake configure with generator '" + plan.Toolchain.Generator + "' for " + DisplayName(plan.Request.PreferredIde) + ".");
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
		if (plan.Kind == BuildWorkspaceOperationKind::InstallHostTool)
		{
			const HostToolInstallerDefinition* installer = FindHostToolInstaller(request.HostToolId);
			if (installer == nullptr)
			{
				AddReadiness(plan, "No launcher installer is registered for host tool '" + request.HostToolId + "'.");
				return;
			}

			const auto status = std::find_if(
			    plan.Toolchain.Items.begin(),
			    plan.Toolchain.Items.end(),
			    [&request](const ToolchainItemStatus& item) { return item.Id == request.HostToolId; });
			if (status != plan.Toolchain.Items.end() && status->State == ToolchainItemState::Found)
			{
				AddReadiness(plan, installer->DisplayName + " is already installed.");
				return;
			}
			if (!CanInstallHostTool(request.HostToolId, plan.Toolchain))
			{
				AddReadiness(plan, "The registered launcher installer for " + installer->DisplayName + " is unavailable on this machine.");
				return;
			}

			AddPlannedEffect(plan, installer->InstallEffect);
			plan.CanRun = true;
			return;
		}

		if (plan.Kind != BuildWorkspaceOperationKind::SyncLevels && !plan.Toolchain.RequiredToolsAvailable)
		{
			AddReadiness(plan, "Required host prerequisites are missing or unsupported.");
			return;
		}

		const bool sourceDependenciesMissing = HasIncompleteEnabledSourceDependencies(plan);
		const bool needsConfigure = BuildWorkspaceOperationRequiresConfigureStep(plan);
		switch (plan.Kind)
		{
			case BuildWorkspaceOperationKind::SyncCode:
				if (!RequireConfigurePrerequisites(plan))
				{
					return;
				}
				if (sourceDependenciesMissing)
				{
					AddPlannedEffect(
					    plan,
					    "Repair incomplete enabled source dependency caches before the next local rebuild or cook workflow.");
				}
				if (needsConfigure)
				{
					AddConfigureStep(plan);
				}
				else
				{
					AddPlannedEffect(plan, "Repository dependency configure is current; no configure step is required.");
				}
				plan.CanRun = true;
				return;
			case BuildWorkspaceOperationKind::SyncLevels:
				if (plan.Toolchain.CMakePath.empty())
				{
					AddReadiness(plan, "CMake is required to acquire selected level asset packs.");
					return;
				}
				AddPlannedEffect(
				    plan,
				    "Acquire asset packs referenced by selected maps into gitignored content roots; unselected and disabled packs remain "
				    "untouched.");
				plan.CanRun = true;
				return;
			case BuildWorkspaceOperationKind::GenerateBuildFiles:
				if (!RequireConfigurePrerequisites(plan))
				{
					return;
				}
				if (sourceDependenciesMissing)
				{
					AddPlannedEffect(
					    plan,
					    "Repair incomplete enabled source dependency caches while refreshing generated workspace files.");
				}
				if (RequiresNativeBuildOutputReset(plan.Freshness.State))
				{
					AddPlannedEffect(
					    plan,
					    "Reset compiler-produced outputs that are incompatible with the selected toolchain while preserving source caches "
					    "and cooked content.");
				}
				AddConfigureStep(plan);
				plan.CanRun = true;
				return;
			case BuildWorkspaceOperationKind::OpenIde:
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
				if (!RequireSyncedSourceDependencies(plan))
				{
					return;
				}
				if (!RequireConfigurePrerequisites(plan))
				{
					return;
				}
				if (sourceDependenciesMissing)
				{
					AddPlannedEffect(
					    plan,
					    "Repair incomplete enabled source dependency caches while refreshing generated workspace files.");
				}
				AddConfigureStep(plan);
				AddBuildStep(plan, request.EditorProfile, {"SparkleLauncher"});
				{
					std::vector<std::string> editorTargets = ResolveProjectTargets(request.ContentId, request.EditorProfile);
					if (editorTargets.empty())
					{
						AddReadiness(plan, "No editor build target could be resolved.");
						return;
					}
					AddBuildStep(plan, request.EditorProfile, editorTargets);
				}
				{
					std::vector<std::string> runtimeTargets = ResolveProjectTargets(request.ContentId, request.RuntimeProfile);
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
						AddPlannedEffect(
						    plan,
						    "Refresh generated workspace files, then build launcher, editor/runtime, and enabled cook "
						    "tools in one pass.");
					}
					else
					{
						AddPlannedEffect(
						    plan,
						    "Refresh generated workspace files, then build launcher plus editor/runtime targets. Optional "
						    "cook tools are disabled in this workspace.");
					}
				}
				plan.CanRun = true;
				return;
			case BuildWorkspaceOperationKind::CompileLauncher:
				if (!RequireSyncedSourceDependencies(plan))
				{
					return;
				}
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
				if (!RequireSyncedSourceDependencies(plan))
				{
					return;
				}
				if (!RequireConfigurePrerequisites(plan))
				{
					return;
				}
				if (!RequireCurrentWorkspace(plan))
				{
					return;
				}
				std::vector<std::string> targets = request.SelectedTargets.empty()
				    ? ResolveProjectTargets(request.ContentId, request.EditorProfile)
				    : request.SelectedTargets;
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
				if (!RequireSyncedSourceDependencies(plan))
				{
					return;
				}
				if (!RequireConfigurePrerequisites(plan))
				{
					return;
				}
				if (!RequireCurrentWorkspace(plan))
				{
					return;
				}
				std::vector<std::string> targets = request.SelectedTargets.empty()
				    ? ResolveProjectTargets(request.ContentId, request.RuntimeProfile)
				    : request.SelectedTargets;
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
				if (!RequireSyncedSourceDependencies(plan))
				{
					return;
				}
				if (!RequireConfigurePrerequisites(plan))
				{
					return;
				}
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
			case BuildWorkspaceOperationKind::InstallHostTool:
				return;
		}
	}

	const std::vector<BuildWorkspaceOperationDefinition>& GetBuildWorkspaceOperationDefinitions()
	{
		static const std::vector<BuildWorkspaceOperationDefinition> definitions = {
		    {BuildWorkspaceOperationKind::SyncCode,
		        "workspace.sync-code",
		        "Sync",
		        std::string(ArtifactNaming::kActionSyncSourceDependencies),
		        "Download enabled repository dependencies and refresh workspace configure state; registered host-tool actions remain "
		        "available "
		        "on the same page."},
		    {BuildWorkspaceOperationKind::SyncLevels,
		        "workspace.sync-levels",
		        "Sync",
		        "Sync Levels",
		        "Select levels and acquire their asset packs without changing code or SDK dependencies."},
		    {BuildWorkspaceOperationKind::GenerateBuildFiles,
		        "workspace.generate-build-files",
		        "Build",
		        std::string(ArtifactNaming::kActionGenerateBuildFiles),
		        "Refresh generated CMake and IDE build files for the selected generator, platform, toolset, and Qt kit."},
		    {BuildWorkspaceOperationKind::OpenIde,
		        "workspace.open-ide",
		        "Launch",
		        "Open IDE",
		        "Open the selected IDE after generated build files are current."},
		    {BuildWorkspaceOperationKind::BuildAll,
		        "workspace.build-all",
		        "Build",
		        "Build All",
		        "Refresh generated workspace files, then rebuild launcher, editor/runtime targets, and enabled cook tools."},
		    {BuildWorkspaceOperationKind::CompileLauncher,
		        "launcher.build.self",
		        "Build",
		        "Build Launcher",
		        "Optional local rebuild of Sparkle Launcher for development or customization."},
		    {BuildWorkspaceOperationKind::CompileEditor,
		        "workspace.build.editor",
		        "Build",
		        "Build Editor",
		        "Optional local rebuild of the editor target."},
		    {BuildWorkspaceOperationKind::CompileRuntime,
		        "workspace.build.runtime",
		        "Build",
		        "Build Runtime",
		        "Optional local rebuild of the runtime target."},
		    {BuildWorkspaceOperationKind::BuildCookTools,
		        "cook.tools.prepare",
		        "Build",
		        "Build Cooking Tools",
		        "Optional local build of tools required by recook workflows."},
		    {BuildWorkspaceOperationKind::InstallHostTool,
		        "workspace.install-host-tool",
		        "Setup",
		        "Install Host Tool",
		        "Install a registered host tool through its launcher-owned installation provider."},
		};
		return definitions;
	}

	std::optional<BuildWorkspaceOperationDefinition> FindBuildWorkspaceOperationDefinition(std::string_view operationId)
	{
		const std::vector<BuildWorkspaceOperationDefinition>& definitions = GetBuildWorkspaceOperationDefinitions();
		const auto found = std::find_if(
		    definitions.begin(),
		    definitions.end(),
		    [operationId](const BuildWorkspaceOperationDefinition& definition) { return definition.Id == operationId; });
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
		plan.Operation.Inputs.push_back({"content", request.ContentId});
		plan.Operation.Inputs.push_back({"editorProfile", request.EditorProfile});
		plan.Operation.Inputs.push_back({"runtimeProfile", request.RuntimeProfile});
		plan.Operation.Inputs.push_back({"workspaceIde", WorkspaceIdeCommandLineValue(request.PreferredIde)});
		plan.Operation.Inputs.push_back({"workspaceCompiler", WorkspaceCompilerCommandLineValue(request.Compiler)});
		if (!request.SourceDependencyId.empty())
		{
			plan.Operation.Inputs.push_back({"sourceDependency", request.SourceDependencyId});
		}
		if (!request.HostToolId.empty())
		{
			plan.Operation.Inputs.push_back({"hostTool", request.HostToolId});
		}
		plan.Operation.LogPath = GetLauncherOperationLogPath(request.RepositoryRoot, definition->Id, "Latest.txt");
		plan.Request = request;
		plan.Toolchain = DetectBuildToolchain(request.RepositoryRoot, request.PreferredIde, request.Compiler);
		plan.Freshness = CheckBuildFilesFreshness(request.RepositoryRoot, plan.Toolchain);
		plan.SourceDependencies = InspectSourceDependencyCache(GetBuildDirectory(request.RepositoryRoot) / "_deps");

		AddReadiness(
		    plan,
		    plan.Toolchain.RequiredToolsAvailable ? "Required toolchain is available." : "Required toolchain is incomplete.");
		AddReadiness(plan, plan.Freshness.Summary);
		if (!plan.SourceDependencies.AllEnabledDependenciesReady)
		{
			for (const std::string& message : plan.SourceDependencies.ReadinessMessages)
			{
				AddReadiness(plan, message);
			}
		}
		PopulatePlanSteps(plan, request);
		if (plan.CanRun)
		{
			try
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
			catch (const Diagnostics::Error& error)
			{
				plan.CanRun = false;
				plan.Steps.clear();
				AddReadiness(plan, std::string("Operation planning failed: ") + error.what());
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
