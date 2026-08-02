#include "MaintenanceOperationProcessRequests.h"

#include "SparkleLauncher/LauncherPaths.h"

#include <algorithm>
#include <utility>

namespace SparkleLauncher
{
	static void AddCleanStep(
	    std::vector<MaintenanceOperationProcessStep>& steps,
	    std::string id,
	    std::string displayName,
	    std::filesystem::path path,
	    MaintenanceCleanBehavior behavior)
	{
		MaintenanceOperationProcessStep step;
		step.Id = std::move(id);
		step.DisplayName = std::move(displayName);
		step.DestructivePath = std::move(path);
		step.CleanBehavior = behavior;
		step.DeletesGeneratedOutput = true;
		steps.push_back(std::move(step));
	}

	static void AddContentGeneratedCleanSteps(
	    std::vector<MaintenanceOperationProcessStep>& steps,
	    const MaintenanceOperationPlan& plan,
	    bool includeBuild,
	    bool includeLogs,
	    bool includeState)
	{
		const std::filesystem::path contentPath = plan.RepositoryRoot / "Projects" / plan.Request.ContentId;
		if (includeBuild)
		{
			AddCleanStep(
			    steps,
			    "clean-content-build",
			    "Clean content build tree",
			    contentPath / "build",
			    MaintenanceCleanBehavior::RemovePath);
		}
		if (includeLogs)
		{
			AddCleanStep(steps, "clean-content-logs", "Clean content logs", contentPath / "logs", MaintenanceCleanBehavior::RemovePath);
		}
		if (includeState)
		{
			AddCleanStep(
			    steps,
			    "clean-content-imgui",
			    "Clean content ImGui state",
			    contentPath / "imgui.ini",
			    MaintenanceCleanBehavior::RemovePath);
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

	static void AddCleanStepsForScope(
	    std::vector<MaintenanceOperationProcessStep>& steps,
	    const MaintenanceOperationPlan& plan,
	    CleanScope scope)
	{
		switch (scope)
		{
			case CleanScope::CookedOutputs:
				AddCleanStep(
				    steps,
				    "clean-cooked",
				    "Clean cooked content",
				    GetCookedProjectDirectory(plan.RepositoryRoot, plan.Request.ContentId),
				    MaintenanceCleanBehavior::RemovePath);
				return;
			case CleanScope::BuildTree:
				AddCleanStep(
				    steps,
				    "clean-build-tree",
				    "Clean build tree except dependency cache",
				    GetBuildDirectory(plan.RepositoryRoot),
				    MaintenanceCleanBehavior::RemoveBuildDirectoryContentsPreservingDependencies);
				AddCleanStep(
				    steps,
				    "clean-root-generated",
				    "Clean root CMake and Visual Studio generated files",
				    plan.RepositoryRoot,
				    MaintenanceCleanBehavior::RemoveRootGeneratedFiles);
				AddContentGeneratedCleanSteps(steps, plan, true, false, false);
				return;
			case CleanScope::ArtifactOutputs:
				AddCleanStep(
				    steps,
				    "clean-artifacts",
				    "Clean generated artifacts",
				    GetArtifactDirectory(plan.RepositoryRoot),
				    MaintenanceCleanBehavior::RemovePath);
				return;
			case CleanScope::PackageOutputs:
				AddCleanStep(
				    steps,
				    "clean-packages",
				    "Clean packaged outputs",
				    plan.RepositoryRoot / "dist",
				    MaintenanceCleanBehavior::RemovePath);
				return;
			case CleanScope::WorkspaceState:
				AddCleanStep(
				    steps,
				    "clean-dot-vs",
				    "Clean Visual Studio workspace state",
				    plan.RepositoryRoot / ".vs",
				    MaintenanceCleanBehavior::RemovePath);
				AddCleanStep(
				    steps,
				    "clean-dot-vscode",
				    "Clean VS Code workspace state",
				    plan.RepositoryRoot / ".vscode",
				    MaintenanceCleanBehavior::RemovePath);
				AddCleanStep(
				    steps,
				    "clean-dot-idea",
				    "Clean Rider workspace state",
				    plan.RepositoryRoot / ".idea",
				    MaintenanceCleanBehavior::RemovePath);
				AddCleanStep(
				    steps,
				    "clean-root-imgui",
				    "Clean root ImGui state",
				    plan.RepositoryRoot / "imgui.ini",
				    MaintenanceCleanBehavior::RemovePath);
				AddContentGeneratedCleanSteps(steps, plan, false, false, true);
				return;
			case CleanScope::ShaderCache:
				AddCleanStep(
				    steps,
				    "clean-shader-cache",
				    "Clean shader cache",
				    GetBuildDirectory(plan.RepositoryRoot) / "Cache" / "Shaders",
				    MaintenanceCleanBehavior::RemovePath);
				return;
			case CleanScope::ThirdPartyDependencyCache:
				AddCleanStep(
				    steps,
				    "clean-dependency-cache",
				    "Clean source dependency cache",
				    GetBuildDirectory(plan.RepositoryRoot) / "_deps",
				    MaintenanceCleanBehavior::RemovePath);
				return;
			case CleanScope::Logs:
				AddCleanStep(
				    steps,
				    "clean-root-logs",
				    "Clean repository logs",
				    plan.RepositoryRoot / "logs",
				    MaintenanceCleanBehavior::RemovePath);
				AddCleanStep(
				    steps,
				    "clean-launcher-logs",
				    "Clean launcher logs",
				    GetLauncherStatePaths(plan.RepositoryRoot).LogsDirectory,
				    MaintenanceCleanBehavior::RemovePath);
				AddContentGeneratedCleanSteps(steps, plan, false, true, false);
				return;
			case CleanScope::PristineGeneratedWorkspace:
				AddCleanStep(
				    steps,
				    "clean-build",
				    "Clean build tree",
				    GetBuildDirectory(plan.RepositoryRoot),
				    MaintenanceCleanBehavior::RemovePath);
				AddCleanStep(
				    steps,
				    "clean-artifacts",
				    "Clean development artifacts",
				    GetArtifactDirectory(plan.RepositoryRoot),
				    MaintenanceCleanBehavior::RemovePath);
				AddCleanStep(
				    steps,
				    "clean-dist",
				    "Clean package outputs",
				    plan.RepositoryRoot / "dist",
				    MaintenanceCleanBehavior::RemovePath);
				AddCleanStep(
				    steps,
				    "clean-dot-vs",
				    "Clean Visual Studio workspace state",
				    plan.RepositoryRoot / ".vs",
				    MaintenanceCleanBehavior::RemovePath);
				AddCleanStep(
				    steps,
				    "clean-dot-vscode",
				    "Clean VS Code workspace state",
				    plan.RepositoryRoot / ".vscode",
				    MaintenanceCleanBehavior::RemovePath);
				AddCleanStep(
				    steps,
				    "clean-dot-idea",
				    "Clean Rider workspace state",
				    plan.RepositoryRoot / ".idea",
				    MaintenanceCleanBehavior::RemovePath);
				AddCleanStep(
				    steps,
				    "clean-logs",
				    "Clean repository logs",
				    plan.RepositoryRoot / "logs",
				    MaintenanceCleanBehavior::RemovePath);
				AddCleanStep(
				    steps,
				    "clean-launcher-state",
				    "Clean launcher state",
				    GetLauncherStatePaths(plan.RepositoryRoot).RootDirectory,
				    MaintenanceCleanBehavior::RemovePath);
				AddCleanStep(
				    steps,
				    "clean-root-imgui",
				    "Clean root ImGui state",
				    plan.RepositoryRoot / "imgui.ini",
				    MaintenanceCleanBehavior::RemovePath);
				AddCleanStep(
				    steps,
				    "clean-root-generated",
				    "Clean root CMake and Visual Studio generated files",
				    plan.RepositoryRoot,
				    MaintenanceCleanBehavior::RemoveRootGeneratedFiles);
				AddContentGeneratedCleanSteps(steps, plan, true, true, true);
				return;
		}
	}

	static void AddCleanSteps(std::vector<MaintenanceOperationProcessStep>& steps, const MaintenanceOperationPlan& plan)
	{
		if (!plan.Request.RequestedCleanTargets.empty())
		{
			for (const MaintenanceCleanPathSpec& target : plan.Request.RequestedCleanTargets)
			{
				AddCleanStep(
				    steps,
				    "clean-explicit-target",
				    "Clean " + target.DisplayName,
				    target.Path,
				    MaintenanceCleanBehavior::RemovePath);
			}
			return;
		}

		for (const CleanScope scope : ResolveRequestedCleanScopes(plan.Request))
		{
			AddCleanStepsForScope(steps, plan, scope);
		}
	}

	std::vector<MaintenanceOperationProcessStep> BuildMaintenanceProcessStepsForPlan(const MaintenanceOperationPlan& plan)
	{
		std::vector<MaintenanceOperationProcessStep> steps;
		if (!plan.CanRun)
		{
			return steps;
		}

		switch (plan.Kind)
		{
			case MaintenanceOperationKind::CleanWorkspace:
				AddCleanSteps(steps, plan);
				return steps;
		}

		return steps;
	}
}
