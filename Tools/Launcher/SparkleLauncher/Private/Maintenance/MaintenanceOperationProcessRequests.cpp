#include "MaintenanceOperationProcessRequests.h"

#include "CMakeWorkflowProcessRequests.h"
#include "SparkleLauncher/LauncherPaths.h"

#include <algorithm>
#include <span>
#include <system_error>
#include <utility>

namespace SparkleLauncher
{
	static constexpr std::size_t kFormatBatchSize = 80;

	static void AddProcessStep(std::vector<MaintenanceOperationProcessStep>& steps, std::string id, std::string displayName, ProcessRequest request)
	{
		MaintenanceOperationProcessStep step;
		step.Id = std::move(id);
		step.DisplayName = std::move(displayName);
		step.Request = std::move(request);
		steps.push_back(std::move(step));
	}

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
		step.HasProcessRequest = false;
		step.DeletesGeneratedOutput = true;
		steps.push_back(std::move(step));
	}

	static void AddProjectGeneratedCleanSteps(
	    std::vector<MaintenanceOperationProcessStep>& steps,
	    const MaintenanceOperationPlan& plan,
	    bool includeBuild,
	    bool includeLogs,
	    bool includeState)
	{
		std::error_code errorCode;
		const std::filesystem::path projectsDirectory = plan.RepositoryRoot / "Projects";
		if (!std::filesystem::is_directory(projectsDirectory, errorCode))
		{
			return;
		}

		for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(projectsDirectory, errorCode))
		{
			if (!entry.is_directory(errorCode))
			{
				continue;
			}

			const std::string projectName = entry.path().filename().string();
			if (includeBuild)
			{
				AddCleanStep(steps, "clean-project-build", "Clean project build tree " + projectName, entry.path() / "build", MaintenanceCleanBehavior::RemovePath);
			}
			if (includeLogs)
			{
				AddCleanStep(steps, "clean-project-logs", "Clean project logs " + projectName, entry.path() / "logs", MaintenanceCleanBehavior::RemovePath);
			}
			if (includeState)
			{
				AddCleanStep(steps, "clean-project-imgui", "Clean project ImGui state " + projectName, entry.path() / "imgui.ini", MaintenanceCleanBehavior::RemovePath);
			}
		}
	}

	static ProcessRequest MakeClangFormatRequest(
	    const MaintenanceOperationPlan& plan,
	    std::span<const std::filesystem::path> files,
	    std::size_t batchIndex)
	{
		ProcessRequest process;
		process.ExecutablePath = plan.Toolchain.ClangFormatPath;
		process.WorkingDirectory = plan.RepositoryRoot;
		process.LogPath = GetLauncherOperationLogPath(plan.RepositoryRoot, plan.Operation.Id, "ClangFormat-" + std::to_string(batchIndex + 1) + ".txt");
		process.Arguments.push_back("-style=file");
		if (plan.Request.RequestedFormatMode == FormatMode::Check)
		{
			process.Arguments.push_back("--dry-run");
			process.Arguments.push_back("--Werror");
		}
		else
		{
			process.Arguments.push_back("-i");
		}

		for (const std::filesystem::path& file : files)
		{
			process.Arguments.push_back(file.string());
		}
		return process;
	}

	static void AddFormatSteps(std::vector<MaintenanceOperationProcessStep>& steps, const MaintenanceOperationPlan& plan)
	{
		std::size_t batchIndex = 0;
		for (std::size_t firstFile = 0; firstFile < plan.FormatSourceFiles.size(); firstFile += kFormatBatchSize)
		{
			const std::size_t lastFile = std::min(firstFile + kFormatBatchSize, plan.FormatSourceFiles.size());
			const std::span<const std::filesystem::path> batch(plan.FormatSourceFiles.data() + firstFile, lastFile - firstFile);
			AddProcessStep(
			    steps,
			    "clang-format-batch",
			    std::string(plan.Request.RequestedFormatMode == FormatMode::Check ? "Check" : "Apply") + " clang-format batch " + std::to_string(batchIndex + 1),
			    MakeClangFormatRequest(plan, batch, batchIndex));
			++batchIndex;
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
			AddCleanStep(steps, "clean-all-cooked", "Clean all cooked outputs", GetBuildDirectory(plan.RepositoryRoot) / "Cooked", MaintenanceCleanBehavior::RemovePath);
			return;
		case CleanScope::BuildTree:
			AddCleanStep(steps, "clean-build-tree", "Clean build tree except dependency cache", GetBuildDirectory(plan.RepositoryRoot), MaintenanceCleanBehavior::RemoveBuildDirectoryContentsPreservingDependencies);
			AddCleanStep(steps, "clean-dot-vs", "Clean Visual Studio workspace state", plan.RepositoryRoot / ".vs", MaintenanceCleanBehavior::RemovePath);
			AddCleanStep(steps, "clean-root-generated", "Clean root CMake and Visual Studio generated files", plan.RepositoryRoot, MaintenanceCleanBehavior::RemoveRootGeneratedFiles);
			AddProjectGeneratedCleanSteps(steps, plan, true, false, false);
			return;
		case CleanScope::ShaderCache:
			AddCleanStep(steps, "clean-shader-cache", "Clean shader cache", GetBuildDirectory(plan.RepositoryRoot) / "Cache" / "Shaders", MaintenanceCleanBehavior::RemovePath);
			return;
		case CleanScope::ThirdPartyDependencyCache:
			AddCleanStep(steps, "clean-dependency-cache", "Clean third-party dependency cache", GetBuildDirectory(plan.RepositoryRoot) / "_deps", MaintenanceCleanBehavior::RemovePath);
			return;
		case CleanScope::Logs:
			AddCleanStep(steps, "clean-root-logs", "Clean repository logs", plan.RepositoryRoot / "logs", MaintenanceCleanBehavior::RemovePath);
			AddCleanStep(steps, "clean-launcher-logs", "Clean launcher logs", GetLauncherStatePaths(plan.RepositoryRoot).LogsDirectory, MaintenanceCleanBehavior::RemovePath);
			AddProjectGeneratedCleanSteps(steps, plan, false, true, false);
			return;
		case CleanScope::PristineGeneratedWorkspace:
			AddCleanStep(steps, "clean-build", "Clean build tree", GetBuildDirectory(plan.RepositoryRoot), MaintenanceCleanBehavior::RemovePath);
			AddCleanStep(steps, "clean-dot-vs", "Clean Visual Studio workspace state", plan.RepositoryRoot / ".vs", MaintenanceCleanBehavior::RemovePath);
			AddCleanStep(steps, "clean-dot-vscode", "Clean VS Code workspace state", plan.RepositoryRoot / ".vscode", MaintenanceCleanBehavior::RemovePath);
			AddCleanStep(steps, "clean-logs", "Clean repository logs", plan.RepositoryRoot / "logs", MaintenanceCleanBehavior::RemovePath);
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
		if (!plan.CanRun && plan.Kind != MaintenanceOperationKind::CleanWorkspace)
		{
			return steps;
		}

		switch (plan.Kind)
		{
		case MaintenanceOperationKind::RunClangFormat:
			AddFormatSteps(steps, plan);
			return steps;
		case MaintenanceOperationKind::CleanWorkspace:
			AddCleanSteps(steps, plan);
			return steps;
		}

		return steps;
	}
}
