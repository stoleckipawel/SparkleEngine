#include "MaintenanceOperationProcessRequests.h"

#include "Core/Public/Strings/StringUtils.h"
#include "SparkleLauncher/LauncherPaths.h"

#include <algorithm>
#include <system_error>
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

	static std::vector<std::filesystem::path> CollectProjectDirectories(const std::filesystem::path& repositoryRoot)
	{
		std::vector<std::filesystem::path> projects;
		std::error_code errorCode;
		const std::filesystem::path projectsDirectory = repositoryRoot / "Projects";
		if (!std::filesystem::is_directory(projectsDirectory, errorCode))
		{
			return projects;
		}

		for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(projectsDirectory, errorCode))
		{
			if (entry.is_directory(errorCode))
			{
				projects.push_back(entry.path());
			}
			errorCode.clear();
		}
		std::ranges::sort(projects, [](const std::filesystem::path& left, const std::filesystem::path& right) {
			return Strings::ToLowerCopy(left.filename().string()) < Strings::ToLowerCopy(right.filename().string());
		});
		return projects;
	}

	static void AddProjectGeneratedCleanSteps(
	    std::vector<MaintenanceOperationProcessStep>& steps,
	    const MaintenanceOperationPlan& plan,
	    bool includeBuild,
	    bool includeLogs,
	    bool includeState)
	{
		for (const std::filesystem::path& projectPath : CollectProjectDirectories(plan.RepositoryRoot))
		{
			const std::string projectName = projectPath.filename().string();
			if (includeBuild)
			{
				AddCleanStep(steps, "clean-project-build", "Clean project build tree " + projectName, projectPath / "build", MaintenanceCleanBehavior::RemovePath);
			}
			if (includeLogs)
			{
				AddCleanStep(steps, "clean-project-logs", "Clean project logs " + projectName, projectPath / "logs", MaintenanceCleanBehavior::RemovePath);
			}
			if (includeState)
			{
				AddCleanStep(steps, "clean-project-imgui", "Clean project ImGui state " + projectName, projectPath / "imgui.ini", MaintenanceCleanBehavior::RemovePath);
			}
		}
	}

	static void AddAllCookedCleanSteps(std::vector<MaintenanceOperationProcessStep>& steps, const MaintenanceOperationPlan& plan)
	{
		AddCleanStep(steps, "clean-shared-cooked", "Clean shared cooked outputs", GetSharedCookedProjectDirectory(plan.RepositoryRoot), MaintenanceCleanBehavior::RemovePath);

		for (const std::filesystem::path& projectPath : CollectProjectDirectories(plan.RepositoryRoot))
		{
			const std::string projectName = projectPath.filename().string();
			if (projectName == "TemplateProject")
			{
				continue;
			}
			AddCleanStep(steps, "clean-project-cooked", "Clean cooked outputs " + projectName, GetCookedProjectDirectory(plan.RepositoryRoot, projectName), MaintenanceCleanBehavior::RemovePath);
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

	static void AddCleanStepsForScope(std::vector<MaintenanceOperationProcessStep>& steps, const MaintenanceOperationPlan& plan, CleanScope scope)
	{
		switch (scope)
		{
		case CleanScope::SelectedProjectCookedOutputs:
			AddCleanStep(steps, "clean-selected-project-cooked", "Clean selected project cooked outputs", GetCookedProjectDirectory(plan.RepositoryRoot, plan.Request.ProjectId), MaintenanceCleanBehavior::RemovePath);
			return;
		case CleanScope::AllCookedOutputs:
			AddAllCookedCleanSteps(steps, plan);
			return;
		case CleanScope::BuildTree:
			AddCleanStep(steps, "clean-build-tree", "Clean build tree except dependency cache", GetBuildDirectory(plan.RepositoryRoot), MaintenanceCleanBehavior::RemoveBuildDirectoryContentsPreservingDependencies);
			AddCleanStep(steps, "clean-root-generated", "Clean root CMake and Visual Studio generated files", plan.RepositoryRoot, MaintenanceCleanBehavior::RemoveRootGeneratedFiles);
			AddProjectGeneratedCleanSteps(steps, plan, true, false, false);
			return;
		case CleanScope::ArtifactOutputs:
			AddCleanStep(steps, "clean-artifacts", "Clean generated artifacts", GetArtifactDirectory(plan.RepositoryRoot), MaintenanceCleanBehavior::RemovePath);
			return;
		case CleanScope::PackageOutputs:
			AddCleanStep(steps, "clean-packages", "Clean packaged outputs", plan.RepositoryRoot / "dist", MaintenanceCleanBehavior::RemovePath);
			return;
		case CleanScope::WorkspaceState:
			AddCleanStep(steps, "clean-dot-vs", "Clean Visual Studio workspace state", plan.RepositoryRoot / ".vs", MaintenanceCleanBehavior::RemovePath);
			AddCleanStep(steps, "clean-dot-vscode", "Clean VS Code workspace state", plan.RepositoryRoot / ".vscode", MaintenanceCleanBehavior::RemovePath);
			AddCleanStep(steps, "clean-dot-idea", "Clean Rider workspace state", plan.RepositoryRoot / ".idea", MaintenanceCleanBehavior::RemovePath);
			AddCleanStep(steps, "clean-root-imgui", "Clean root ImGui state", plan.RepositoryRoot / "imgui.ini", MaintenanceCleanBehavior::RemovePath);
			AddProjectGeneratedCleanSteps(steps, plan, false, false, true);
			return;
		case CleanScope::ShaderCache:
			AddCleanStep(steps, "clean-shader-cache", "Clean shader cache", GetBuildDirectory(plan.RepositoryRoot) / "Cache" / "Shaders", MaintenanceCleanBehavior::RemovePath);
			return;
		case CleanScope::ThirdPartyDependencyCache:
			AddCleanStep(steps, "clean-dependency-cache", "Clean source dependency cache", GetBuildDirectory(plan.RepositoryRoot) / "_deps", MaintenanceCleanBehavior::RemovePath);
			return;
		case CleanScope::Logs:
			AddCleanStep(steps, "clean-root-logs", "Clean repository logs", plan.RepositoryRoot / "logs", MaintenanceCleanBehavior::RemovePath);
			AddCleanStep(steps, "clean-launcher-logs", "Clean launcher logs", GetLauncherStatePaths(plan.RepositoryRoot).LogsDirectory, MaintenanceCleanBehavior::RemovePath);
			AddProjectGeneratedCleanSteps(steps, plan, false, true, false);
			return;
		case CleanScope::PristineGeneratedWorkspace:
			AddCleanStep(steps, "clean-build", "Clean build tree", GetBuildDirectory(plan.RepositoryRoot), MaintenanceCleanBehavior::RemovePath);
			AddCleanStep(steps, "clean-artifacts", "Clean development artifacts", GetArtifactDirectory(plan.RepositoryRoot), MaintenanceCleanBehavior::RemovePath);
			AddCleanStep(steps, "clean-dist", "Clean package outputs", plan.RepositoryRoot / "dist", MaintenanceCleanBehavior::RemovePath);
			AddCleanStep(steps, "clean-dot-vs", "Clean Visual Studio workspace state", plan.RepositoryRoot / ".vs", MaintenanceCleanBehavior::RemovePath);
			AddCleanStep(steps, "clean-dot-vscode", "Clean VS Code workspace state", plan.RepositoryRoot / ".vscode", MaintenanceCleanBehavior::RemovePath);
			AddCleanStep(steps, "clean-dot-idea", "Clean Rider workspace state", plan.RepositoryRoot / ".idea", MaintenanceCleanBehavior::RemovePath);
			AddCleanStep(steps, "clean-logs", "Clean repository logs", plan.RepositoryRoot / "logs", MaintenanceCleanBehavior::RemovePath);
			AddCleanStep(steps, "clean-launcher-state", "Clean launcher state", GetLauncherStatePaths(plan.RepositoryRoot).RootDirectory, MaintenanceCleanBehavior::RemovePath);
			AddCleanStep(steps, "clean-root-imgui", "Clean root ImGui state", plan.RepositoryRoot / "imgui.ini", MaintenanceCleanBehavior::RemovePath);
			AddCleanStep(steps, "clean-root-generated", "Clean root CMake and Visual Studio generated files", plan.RepositoryRoot, MaintenanceCleanBehavior::RemoveRootGeneratedFiles);
			AddProjectGeneratedCleanSteps(steps, plan, true, true, true);
			return;
		}
	}

	static void AddCleanSteps(std::vector<MaintenanceOperationProcessStep>& steps, const MaintenanceOperationPlan& plan)
	{
		if (!plan.Request.RequestedCleanTargets.empty())
		{
			for (const MaintenanceCleanPathSpec& target : plan.Request.RequestedCleanTargets)
			{
				AddCleanStep(steps, "clean-explicit-target", "Clean " + target.DisplayName, target.Path, MaintenanceCleanBehavior::RemovePath);
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
