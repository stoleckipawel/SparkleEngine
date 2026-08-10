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

	static bool HasSelectedScope(const BuildWorkspaceOperationRequest& request, BuildWorkspaceScope scope)
	{
		return std::find(request.SelectedScopes.begin(), request.SelectedScopes.end(), scope) != request.SelectedScopes.end();
	}

	static void AppendTargets(std::vector<std::string>& destination, const std::vector<std::string>& targets)
	{
		destination.insert(destination.end(), targets.begin(), targets.end());
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

		if (!plan.Toolchain.RequiredToolsAvailable)
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
			case BuildWorkspaceOperationKind::BuildWorkspace:
			{
				if (request.SelectedScopes.empty())
				{
					AddReadiness(plan, "Select at least one workspace build scope.");
					return;
				}
				if (!RequireSyncedSourceDependencies(plan))
				{
					return;
				}
				if (!RequireConfigurePrerequisites(plan))
				{
					return;
				}
				if (needsConfigure)
				{
					if (RequiresNativeBuildOutputReset(plan.Freshness.State))
					{
						AddPlannedEffect(
						    plan,
						    "Reset compiler-produced outputs that are incompatible with the selected toolchain while preserving source "
						    "caches "
						    "and cooked content.");
					}
					AddConfigureStep(plan);
				}

				std::vector<std::string> editorProfileTargets;
				if (HasSelectedScope(request, BuildWorkspaceScope::Launcher))
				{
					editorProfileTargets.push_back("SparkleLauncher");
				}
				if (HasSelectedScope(request, BuildWorkspaceScope::Editor))
				{
					const std::vector<std::string> editorTargets = ResolveProjectTargets(request.ContentId, request.EditorProfile);
					if (editorTargets.empty())
					{
						AddReadiness(plan, "No editor build target could be resolved.");
						return;
					}
					AppendTargets(editorProfileTargets, editorTargets);
				}
				if (HasSelectedScope(request, BuildWorkspaceScope::CookTools))
				{
					const std::vector<std::string> cookToolTargets = GetEnabledCookToolTargets();
					if (cookToolTargets.empty())
					{
						AddReadiness(plan, "No cook-tool targets are enabled in this workspace configuration.");
						return;
					}
					AppendTargets(editorProfileTargets, cookToolTargets);
				}
				if (!editorProfileTargets.empty())
				{
					AddBuildStep(plan, request.EditorProfile, editorProfileTargets);
				}

				if (HasSelectedScope(request, BuildWorkspaceScope::Runtime))
				{
					const std::vector<std::string> runtimeTargets = ResolveProjectTargets(request.ContentId, request.RuntimeProfile);
					if (runtimeTargets.empty())
					{
						AddReadiness(plan, "No runtime build target could be resolved.");
						return;
					}
					AddBuildStep(plan, request.RuntimeProfile, runtimeTargets);
				}
				AddPlannedEffect(plan, "Build the selected workspace products in dependency order using incremental CMake builds.");
				plan.CanRun = true;
				return;
			}
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
		    {BuildWorkspaceOperationKind::GenerateBuildFiles,
		        "workspace.generate-build-files",
		        "Build",
		        std::string(ArtifactNaming::kActionGenerateBuildFiles),
		        "Refresh generated CMake and IDE build files for the selected generator, platform, toolset, and Qt kit."},
		    {BuildWorkspaceOperationKind::BuildWorkspace,
		        "workspace.build",
		        "Build",
		        "Build Workspace",
		        "Build the selected editor, game, cooking-tool, and launcher scopes; refresh generated workspace files when needed."},
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
		if (!request.SelectedScopes.empty())
		{
			std::vector<std::string> scopeIds;
			scopeIds.reserve(request.SelectedScopes.size());
			for (const BuildWorkspaceScope scope : request.SelectedScopes)
			{
				scopeIds.push_back(BuildWorkspaceScopeId(scope));
			}
			std::vector<std::string_view> scopeViews;
			scopeViews.reserve(scopeIds.size());
			for (const std::string& scopeId : scopeIds)
			{
				scopeViews.push_back(scopeId);
			}
			plan.Operation.Inputs.push_back({"buildScopes", Strings::Join(scopeViews, ",")});
		}
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
