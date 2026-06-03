#include "BuildWorkspaceProcessRequests.h"

#include "CMakeWorkflowProcessRequests.h"
#include "SparkleLauncher/BuildProfileCatalog.h"
#include "SparkleLauncher/LauncherPaths.h"

#include <cstdlib>
#include <optional>
#include <string_view>
#include <utility>

namespace SparkleLauncher
{
	static ProcessRequest MakeConfigureRequest(const BuildWorkspaceOperationPlan& plan)
	{
		return MakeCMakeConfigureRequest(plan.RepositoryRoot, plan.Toolchain, plan.Operation.Id, "Configure.txt");
	}

	static ProcessRequest MakeBuildRequest(const BuildWorkspaceOperationPlan& plan, std::string_view profileName, const std::vector<std::string>& targets)
	{
		return MakeCMakeBuildRequest(plan.RepositoryRoot, plan.Toolchain, plan.Operation.Id, profileName, targets, "Build.txt");
	}

	static std::filesystem::path GetCommandProcessorPath()
	{
#if defined(_WIN32)
		if (const char* commandProcessor = std::getenv("ComSpec"); commandProcessor != nullptr && commandProcessor[0] != '\0')
		{
			return std::filesystem::path(commandProcessor);
		}
		if (const char* systemRoot = std::getenv("SystemRoot"); systemRoot != nullptr && systemRoot[0] != '\0')
		{
			return std::filesystem::path(systemRoot) / "System32" / "cmd.exe";
		}
		return "cmd.exe";
#else
		return "xdg-open";
#endif
	}

	static ProcessRequest MakeOpenSolutionRequest(const BuildWorkspaceOperationPlan& plan)
	{
		ProcessRequest process;
		process.WorkingDirectory = plan.RepositoryRoot;
		process.LogPath = GetLauncherOperationLogPath(plan.RepositoryRoot, plan.Operation.Id, "OpenWorkspace.txt");
#if defined(_WIN32)
		process.ExecutablePath = GetCommandProcessorPath();
		if (plan.Request.PreferredIde == WorkspaceIde::Rider)
		{
			process.Arguments = {"/C", "start", "", plan.Toolchain.RiderPath.string(), plan.RepositoryRoot.string()};
		}
		else
		{
			process.Arguments = {"/C", "start", "", plan.Freshness.SolutionPath.string()};
		}
#else
		process.ExecutablePath = "xdg-open";
		process.Arguments = {plan.Request.PreferredIde == WorkspaceIde::Rider ? plan.RepositoryRoot.string() : plan.Freshness.SolutionPath.string()};
#endif
		return process;
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
		return ResolveProjectTargets(plan.Request.ProjectId, profileName);
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

	static void AddConfigureStep(std::vector<BuildWorkspaceProcessStep>& steps, const BuildWorkspaceOperationPlan& plan)
	{
		BuildWorkspaceProcessStep step;
		step.Id = "configure";
		step.DisplayName = "Generate project files";
		step.Request = MakeConfigureRequest(plan);
		step.UpdatesBuildFilesFreshness = true;
		steps.push_back(std::move(step));
	}

	static void AddBuildStep(std::vector<BuildWorkspaceProcessStep>& steps, const BuildWorkspaceOperationPlan& plan, std::string_view profileName, const std::vector<std::string>& targets)
	{
		BuildWorkspaceProcessStep step;
		step.Id = "build";
		step.DisplayName = "Build targets";
		step.Request = MakeBuildRequest(plan, profileName, targets);
		steps.push_back(std::move(step));
	}

	static void AddOpenSolutionStep(std::vector<BuildWorkspaceProcessStep>& steps, const BuildWorkspaceOperationPlan& plan)
	{
		BuildWorkspaceProcessStep step;
		step.Id = "open-solution";
		step.DisplayName = plan.Request.PreferredIde == WorkspaceIde::Rider ? "Open Rider" : "Open Visual Studio";
		step.Request = MakeOpenSolutionRequest(plan);
		steps.push_back(std::move(step));
	}

	std::vector<BuildWorkspaceProcessStep> BuildProcessStepsForPlan(const BuildWorkspaceOperationPlan& plan)
	{
		std::vector<BuildWorkspaceProcessStep> steps;
		if (!plan.Toolchain.RequiredToolsAvailable)
		{
			return steps;
		}

		const bool needsConfigure = plan.Request.ForceConfigure || !plan.Freshness.Current;
		switch (plan.Kind)
		{
		case BuildWorkspaceOperationKind::CheckToolchain:
			return steps;
		case BuildWorkspaceOperationKind::SetupWorkspace:
			if (needsConfigure)
			{
				AddConfigureStep(steps, plan);
			}
			return steps;
		case BuildWorkspaceOperationKind::GenerateSolution:
			AddConfigureStep(steps, plan);
			return steps;
		case BuildWorkspaceOperationKind::OpenSolution:
			AddOpenSolutionStep(steps, plan);
			return steps;
		case BuildWorkspaceOperationKind::BuildAll:
			AddBuildStep(steps, plan, plan.Request.EditorProfile, {"SparkleLauncher"});
			AddBuildStep(steps, plan, plan.Request.EditorProfile, ResolveProjectTargets(plan.Request.ProjectId, plan.Request.EditorProfile));
			AddBuildStep(steps, plan, plan.Request.RuntimeProfile, ResolveProjectTargets(plan.Request.ProjectId, plan.Request.RuntimeProfile));
			{
				const std::vector<std::string> cookToolTargets = GetEnabledCookToolTargets();
				if (!cookToolTargets.empty())
				{
					AddBuildStep(steps, plan, plan.Request.EditorProfile, cookToolTargets);
				}
			}
			return steps;
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
		case BuildWorkspaceOperationKind::AssembleRelease:
			AddBuildStep(steps, plan, plan.Request.EditorProfile, {"sparkle_release_assembly"});
			return steps;
		}

		return steps;
	}
}
