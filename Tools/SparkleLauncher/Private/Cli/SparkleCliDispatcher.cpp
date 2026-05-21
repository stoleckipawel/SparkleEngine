#include "SparkleCliDispatcher.h"

#include "SparkleLauncher/BuildWorkspaceOperations.h"
#include "SparkleLauncher/CookOperations.h"
#include "SparkleLauncher/LaunchOperations.h"
#include "SparkleLauncher/MaintenanceOperations.h"

#include <algorithm>
#include <filesystem>
#include <optional>
#include <ostream>
#include <utility>

namespace SparkleLauncher
{
	int SparkleCliDispatcher::Dispatch(const SparkleCliArguments& arguments, std::ostream& output, std::ostream& error) const
	{
		std::string errorMessage;
		const std::filesystem::path startPath = arguments.StartPath.empty() ? std::filesystem::current_path() : arguments.StartPath;
		const std::optional<RepositoryRoot> repository = TryFindRepositoryRoot(startPath, errorMessage);
		if (!repository.has_value())
		{
			error << errorMessage << "\n";
			return 1;
		}

		const std::vector<SparkleProject> projects = DiscoverProjects(repository->RootPath, errorMessage);
		if (!errorMessage.empty())
		{
			error << errorMessage << "\n";
			return 1;
		}

		const std::string projectId = ChooseProjectId(projects, arguments.ProjectId);
		if (FindBuildWorkspaceOperationDefinition(arguments.OperationId).has_value())
		{
			return DispatchBuild(arguments, *repository, projectId, output);
		}
		if (FindCookOperationDefinition(arguments.OperationId).has_value())
		{
			return DispatchCook(arguments, *repository, projectId, output);
		}
		if (FindMaintenanceOperationDefinition(arguments.OperationId).has_value())
		{
			return DispatchMaintenance(arguments, *repository, projectId, output);
		}
		if (FindLaunchOperationDefinition(arguments.OperationId).has_value())
		{
			return DispatchLaunch(arguments, *repository, projectId, output);
		}

		error << "Sparkle: unknown operation id '" << arguments.OperationId << "'.\n";
		return 1;
	}

	std::string SparkleCliDispatcher::ChooseProjectId(const std::vector<SparkleProject>& projects, std::string_view requestedProjectId) const
	{
		if (!requestedProjectId.empty())
		{
			const auto requestedProject = std::find_if(projects.begin(), projects.end(), [requestedProjectId](const SparkleProject& project) {
				return project.Id == requestedProjectId;
			});
			return requestedProject == projects.end() ? std::string(requestedProjectId) : requestedProject->Id;
		}

		const auto showcaseProject = std::find_if(projects.begin(), projects.end(), [](const SparkleProject& project) {
			return project.Id == "Showcase";
		});
		if (showcaseProject != projects.end())
		{
			return showcaseProject->Id;
		}

		return projects.empty() ? std::string("Showcase") : projects.front().Id;
	}

	int SparkleCliDispatcher::DispatchBuild(const SparkleCliArguments& arguments, const RepositoryRoot& repository, std::string_view projectId, std::ostream& output) const
	{
		BuildWorkspaceOperationRequest request;
		request.RepositoryRoot = repository.RootPath;
		request.ProjectId = projectId;
		request.EditorProfile = arguments.EditorProfile;
		request.RuntimeProfile = arguments.RuntimeProfile;
		request.SelectedTargets = arguments.SelectedTargets;
		request.ForceConfigure = arguments.ForceConfigure;

		BuildWorkspaceOperationPlan plan = PlanBuildWorkspaceOperation(arguments.OperationId, request);
		Output.PrintPlanDetails(plan.Operation, plan.CanRun, plan.ReadinessMessages, plan.PlannedEffects, output);
		if (arguments.DryRun)
		{
			return FinishDryRun(plan.CanRun);
		}

		NativeProcessRunner processRunner;
		const OperationRecord operation = RunBuildWorkspaceOperationPlan(std::move(plan), processRunner, [&output](std::string_view line) { output << line; });
		Output.PrintOperationRecord(operation, output);
		return FinishOperation(operation);
	}

	int SparkleCliDispatcher::DispatchCook(const SparkleCliArguments& arguments, const RepositoryRoot& repository, std::string_view projectId, std::ostream& output) const
	{
		CookOperationRequest request;
		request.RepositoryRoot = repository.RootPath;
		request.ProjectId = projectId;
		request.RuntimeProfile = arguments.RuntimeProfile;
		request.Mode = arguments.RequestedCookMode;
		request.ForceRecookConfirmed = arguments.ForceRecookConfirmed;
		request.ShaderPackages = arguments.ShaderPackages;

		CookOperationPlan plan = PlanCookOperation(arguments.OperationId, request);
		Output.PrintPlanDetails(plan.Operation, plan.CanRun, plan.ReadinessMessages, plan.PlannedEffects, output);
		if (arguments.DryRun)
		{
			return FinishDryRun(plan.CanRun);
		}

		NativeProcessRunner processRunner;
		const OperationRecord operation = RunCookOperationPlan(std::move(plan), processRunner, [&output](std::string_view line) { output << line; });
		Output.PrintOperationRecord(operation, output);
		return FinishOperation(operation);
	}

	int SparkleCliDispatcher::DispatchMaintenance(const SparkleCliArguments& arguments, const RepositoryRoot& repository, std::string_view projectId, std::ostream& output) const
	{
		MaintenanceOperationRequest request;
		request.RepositoryRoot = repository.RootPath;
		request.ProjectId = projectId;
		request.EditorProfile = arguments.EditorProfile;
		request.RequestedFormatMode = arguments.RequestedFormatMode;
		request.RequestedCleanScope = arguments.RequestedCleanScope;
		request.DestructiveActionConfirmed = arguments.CleanConfirmed;
		request.ValidationGroups = arguments.ValidationGroups;
		request.ValidationTargets = arguments.ValidationTargets;

		MaintenanceOperationPlan plan = PlanMaintenanceOperation(arguments.OperationId, request);
		Output.PrintPlanDetails(plan.Operation, plan.CanRun, plan.ReadinessMessages, plan.PlannedEffects, output);
		if (arguments.DryRun)
		{
			return FinishDryRun(plan.CanRun);
		}

		NativeProcessRunner processRunner;
		const OperationRecord operation = RunMaintenanceOperationPlan(std::move(plan), processRunner, [&output](std::string_view line) { output << line; });
		Output.PrintOperationRecord(operation, output);
		return FinishOperation(operation);
	}

	int SparkleCliDispatcher::DispatchLaunch(const SparkleCliArguments& arguments, const RepositoryRoot& repository, std::string_view projectId, std::ostream& output) const
	{
		LaunchOperationRequest request;
		request.RepositoryRoot = repository.RootPath;
		request.ProjectId = projectId;
		request.EditorProfile = arguments.EditorProfile;
		request.RuntimeProfile = arguments.RuntimeProfile;
		request.SmokeBackend = arguments.SmokeBackend;
		request.SmokeFrameLimit = arguments.SmokeFrameLimit;
		request.SmokeTrace = arguments.SmokeTrace;
		request.SmokeSkipLevelSwitching = arguments.SmokeSkipLevelSwitching;

		LaunchOperationPlan plan = PlanLaunchOperation(arguments.OperationId, request);
		Output.PrintPlanDetails(plan.Operation, plan.CanRun, plan.ReadinessMessages, plan.PlannedEffects, output);
		if (arguments.DryRun)
		{
			return FinishDryRun(plan.CanRun);
		}

		NativeProcessRunner processRunner;
		const OperationRecord operation = RunLaunchOperationPlan(std::move(plan), processRunner, [&output](std::string_view line) { output << line; });
		Output.PrintOperationRecord(operation, output);
		return FinishOperation(operation);
	}

	int SparkleCliDispatcher::FinishDryRun(bool canRun) const
	{
		return canRun ? 0 : 2;
	}

	int SparkleCliDispatcher::FinishOperation(const OperationRecord& operation) const
	{
		return operation.Status == OperationStatus::Succeeded ? 0 : 1;
	}
}