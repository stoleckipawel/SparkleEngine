#include "BuildWorkspaceProcessRequests.h"

#include "CMakeWorkflowProcessRequests.h"
#include "HostToolInstaller.h"
#include "Core/Public/Diagnostics/Error.h"
#include "SparkleLauncher/BuildProfileCatalog.h"
#include "SparkleLauncher/LauncherPaths.h"

#include <algorithm>
#include <optional>
#include <string_view>
#include <utility>

namespace SparkleLauncher
{
	static ProcessRequest MakeConfigureRequest(const BuildWorkspaceOperationPlan& plan)
	{
		if (!plan.Request.SourceDependencyId.empty())
		{
			return MakeCMakeDependencySyncRequest(
			    plan.RepositoryRoot,
			    plan.Toolchain,
			    plan.Operation.Id,
			    plan.Request.SourceDependencyId,
			    "SyncSourceDependency.txt");
		}
		return MakeCMakeConfigureRequest(plan.RepositoryRoot, plan.Toolchain, plan.Operation.Id, "Configure.txt");
	}

	static ProcessRequest MakeBuildRequest(
	    const BuildWorkspaceOperationPlan& plan,
	    std::string_view profileName,
	    const std::vector<std::string>& targets)
	{
		return MakeCMakeBuildRequest(plan.RepositoryRoot, plan.Toolchain, plan.Operation.Id, profileName, targets, "Build.txt");
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

	static std::vector<std::string> ResolveBuildTargets(const BuildWorkspaceOperationPlan& plan, std::string_view profileName)
	{
		if (!plan.Request.SelectedTargets.empty())
		{
			return plan.Request.SelectedTargets;
		}
		return ResolveProjectTargets(plan.Request.ContentId, profileName);
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

	static void AddConfigureStep(std::vector<BuildWorkspaceProcessStep>& steps, const BuildWorkspaceOperationPlan& plan)
	{
		BuildWorkspaceProcessStep step;
		step.Id = "configure";
		step.DisplayName = "Generate build files";
		step.Request = MakeConfigureRequest(plan);
		step.UpdatesBuildFilesFreshness = plan.Request.SourceDependencyId.empty();
		steps.push_back(std::move(step));
	}

	static void AddBuildStep(
	    std::vector<BuildWorkspaceProcessStep>& steps,
	    const BuildWorkspaceOperationPlan& plan,
	    std::string_view profileName,
	    const std::vector<std::string>& targets)
	{
		BuildWorkspaceProcessStep step;
		step.Id = "build";
		step.DisplayName = "Build targets";
		step.Request = MakeBuildRequest(plan, profileName, targets);
		steps.push_back(std::move(step));
	}

	static void AddHostToolInstallStep(std::vector<BuildWorkspaceProcessStep>& steps, const BuildWorkspaceOperationPlan& plan)
	{
		std::string errorMessage;
		std::optional<ProcessRequest> request =
		    BuildHostToolInstallRequest(plan.Request.HostToolId, plan.Toolchain, plan.RepositoryRoot, plan.Operation.Id, errorMessage);
		if (!request.has_value())
		{
			throw Diagnostics::Error(errorMessage);
		}

		BuildWorkspaceProcessStep step;
		step.Id = "install-host-tool";
		step.DisplayName = "Install host tool";
		step.Request = std::move(*request);
		steps.push_back(std::move(step));
	}

	std::vector<BuildWorkspaceProcessStep> BuildProcessStepsForPlan(const BuildWorkspaceOperationPlan& plan)
	{
		std::vector<BuildWorkspaceProcessStep> steps;
		if (plan.Kind == BuildWorkspaceOperationKind::InstallHostTool)
		{
			AddHostToolInstallStep(steps, plan);
			return steps;
		}
		if (!plan.Toolchain.RequiredToolsAvailable)
		{
			return steps;
		}

		switch (plan.Kind)
		{
			case BuildWorkspaceOperationKind::SyncCode:
				if (BuildWorkspaceOperationRequiresConfigureStep(plan))
				{
					AddConfigureStep(steps, plan);
				}
				return steps;
			case BuildWorkspaceOperationKind::GenerateBuildFiles:
				AddConfigureStep(steps, plan);
				return steps;
			case BuildWorkspaceOperationKind::BuildWorkspace:
			{
				if (BuildWorkspaceOperationRequiresConfigureStep(plan))
				{
					AddConfigureStep(steps, plan);
				}

				std::vector<std::string> editorProfileTargets;
				if (HasSelectedScope(plan.Request, BuildWorkspaceScope::Launcher))
				{
					editorProfileTargets.push_back("SparkleLauncher");
				}
				if (HasSelectedScope(plan.Request, BuildWorkspaceScope::Editor))
				{
					AppendTargets(editorProfileTargets, ResolveProjectTargets(plan.Request.ContentId, plan.Request.EditorProfile));
				}
				if (HasSelectedScope(plan.Request, BuildWorkspaceScope::CookTools))
				{
					AppendTargets(editorProfileTargets, GetEnabledCookToolTargets());
				}
				if (!editorProfileTargets.empty())
				{
					AddBuildStep(steps, plan, plan.Request.EditorProfile, editorProfileTargets);
				}
				if (HasSelectedScope(plan.Request, BuildWorkspaceScope::Runtime))
				{
					AddBuildStep(
					    steps,
					    plan,
					    plan.Request.RuntimeProfile,
					    ResolveProjectTargets(plan.Request.ContentId, plan.Request.RuntimeProfile));
				}
				return steps;
			}
			case BuildWorkspaceOperationKind::CompileLauncher:
				AddBuildStep(steps, plan, plan.Request.EditorProfile, {"SparkleLauncher"});
				return steps;
			case BuildWorkspaceOperationKind::CompileEditor:
				AddBuildStep(steps, plan, plan.Request.EditorProfile, ResolveBuildTargets(plan, plan.Request.EditorProfile));
				return steps;
			case BuildWorkspaceOperationKind::CompileRuntime:
				AddBuildStep(steps, plan, plan.Request.RuntimeProfile, ResolveBuildTargets(plan, plan.Request.RuntimeProfile));
				return steps;
			case BuildWorkspaceOperationKind::BuildCookTools:
			{
				const std::vector<std::string> cookToolTargets = GetEnabledCookToolTargets();
				if (!cookToolTargets.empty())
				{
					AddBuildStep(steps, plan, plan.Request.EditorProfile, cookToolTargets);
				}
				return steps;
			}
			case BuildWorkspaceOperationKind::InstallHostTool:
				return steps;
		}

		return steps;
	}
}
