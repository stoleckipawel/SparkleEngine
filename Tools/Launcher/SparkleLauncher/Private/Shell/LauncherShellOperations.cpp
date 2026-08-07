#include "LauncherShellOperations.h"

#include "LauncherShellArguments.h"
#include "LauncherShellModel.h"

#include "SparkleLauncher/BuildWorkspaceOperations.h"
#include "SparkleLauncher/CookOperations.h"
#include "SparkleLauncher/LevelOperations.h"
#include "SparkleLauncher/LaunchOperations.h"
#include "SparkleLauncher/MaintenanceOperations.h"
#include "SparkleLauncher/OperationModel.h"
#include "SparkleLauncher/ProcessRunner.h"

#include <ostream>
#include <string>
#include <string_view>

namespace SparkleLauncher
{
	BuildWorkspaceOperationRequest BuildWorkspaceShellRequest(const LauncherShellModel& model)
	{
		BuildWorkspaceOperationRequest request;
		request.RepositoryRoot = model.Repository.RootPath;
		request.ContentId = model.ContentId;
		request.EditorProfile = model.EditorProfile;
		request.RuntimeProfile = model.RuntimeProfile;
		request.PreferredIde = model.WorkspaceIdePreference;
		request.Compiler = model.WorkspaceCompilerPreference;
		request.ForceConfigure = false;
		return request;
	}

	CookOperationRequest BuildCookShellRequest(const LauncherShellModel& model, const LauncherShellArguments& arguments)
	{
		CookOperationRequest request;
		request.RepositoryRoot = model.Repository.RootPath;
		request.ContentId = model.ContentId;
		request.RuntimeProfile = model.RuntimeProfile;
		request.Mode = arguments.RequestedCookMode;
		request.ForceRecookConfirmed = arguments.ForceRecookConfirmed;
		return request;
	}

	LevelOperationRequest BuildLevelShellRequest(const LauncherShellModel& model)
	{
		LevelOperationRequest request;
		request.RepositoryRoot = model.Repository.RootPath;
		request.ContentId = model.ContentId;
		return request;
	}

	MaintenanceOperationRequest BuildMaintenanceShellRequest(const LauncherShellModel& model, const LauncherShellArguments& arguments)
	{
		MaintenanceOperationRequest request;
		request.RepositoryRoot = model.Repository.RootPath;
		request.ContentId = model.ContentId;
		request.EditorProfile = model.EditorProfile;
		request.RequestedCleanScope = arguments.RequestedCleanScope;
		request.DestructiveActionConfirmed = arguments.CleanConfirmed;
		return request;
	}

	LaunchOperationRequest BuildLaunchShellRequest(const LauncherShellModel& model, const LauncherShellArguments& arguments)
	{
		LaunchOperationRequest request;
		request.RepositoryRoot = model.Repository.RootPath;
		request.ContentId = model.ContentId;
		request.EditorProfile = model.EditorProfile;
		request.RuntimeProfile = model.RuntimeProfile;
		request.StartupLevel = arguments.StartupLevel;
		return request;
	}

	void AppendBuildPlanDryRun(LauncherShellModel& model, const BuildWorkspaceOperationPlan& plan)
	{
		model.JobOutput.push_back(plan.Operation.DisplayName + " [" + std::string(plan.CanRun ? "Ready" : "Blocked") + "]");
		model.JobOutput.push_back("Editor profile: " + model.EditorProfile);
		model.JobOutput.push_back("Runtime profile: " + model.RuntimeProfile);
		model.JobOutput.push_back("Workspace IDE: " + DisplayName(model.WorkspaceIdePreference));
		model.JobOutput.push_back("Workspace compiler: " + DisplayName(model.WorkspaceCompilerPreference));
		model.JobOutput.push_back("Latest log: " + plan.Operation.LogPath.string());
		for (const std::string& message : plan.ReadinessMessages)
		{
			model.JobOutput.push_back("Readiness: " + message);
		}
		for (const std::string& effect : plan.PlannedEffects)
		{
			model.JobOutput.push_back("Effect: " + effect);
		}
		model.JobOutput.push_back(plan.Operation.DryRunText);
		RecordLauncherShellActivity(model, plan.Operation.DisplayName + " dry-run planned");
	}

	void AppendCookPlanDryRun(LauncherShellModel& model, const CookOperationPlan& plan)
	{
		model.JobOutput.push_back(plan.Operation.DisplayName + " [" + std::string(plan.CanRun ? "Ready" : "Blocked") + "]");
		model.JobOutput.push_back("Runtime profile: " + model.RuntimeProfile);
		model.JobOutput.push_back("Cook mode: " + ToString(plan.Request.Mode));
		model.JobOutput.push_back("Cooked output: " + plan.CookedOutputDirectory.string());
		model.JobOutput.push_back("Latest log: " + plan.Operation.LogPath.string());
		if (plan.Operation.RequiresConfirmation)
		{
			model.JobOutput.push_back("Confirmation required: " + ToString(plan.Operation.DestructiveScope));
		}
		for (const std::string& message : plan.ReadinessMessages)
		{
			model.JobOutput.push_back("Readiness: " + message);
		}
		for (const std::string& effect : plan.PlannedEffects)
		{
			model.JobOutput.push_back("Effect: " + effect);
		}
		model.JobOutput.push_back(plan.Operation.DryRunText);
		RecordLauncherShellActivity(model, plan.Operation.DisplayName + " dry-run planned");
	}

	void AppendLevelPlanDryRun(LauncherShellModel& model, const LevelOperationPlan& plan)
	{
		model.JobOutput.push_back(plan.Operation.DisplayName + " [" + std::string(plan.CanRun ? "Ready" : "Blocked") + "]");
		model.JobOutput.push_back("Content: " + model.ContentId);
		model.JobOutput.push_back("Latest log: " + plan.Operation.LogPath.string());
		for (const std::string& message : plan.ReadinessMessages)
		{
			model.JobOutput.push_back("Readiness: " + message);
		}
		for (const std::string& effect : plan.PlannedEffects)
		{
			model.JobOutput.push_back("Effect: " + effect);
		}
		model.JobOutput.push_back(plan.Operation.DryRunText);
		RecordLauncherShellActivity(model, plan.Operation.DisplayName + " dry-run planned");
	}

	void AppendMaintenancePlanDryRun(LauncherShellModel& model, const MaintenanceOperationPlan& plan)
	{
		model.JobOutput.push_back(plan.Operation.DisplayName + " [" + std::string(plan.CanRun ? "Ready" : "Blocked") + "]");
		model.JobOutput.push_back("Editor profile: " + model.EditorProfile);
		model.JobOutput.push_back("Clean scope: " + ToString(plan.Request.RequestedCleanScope));
		model.JobOutput.push_back("Latest log: " + plan.Operation.LogPath.string());
		if (plan.Operation.RequiresConfirmation)
		{
			model.JobOutput.push_back("Confirmation required: " + ToString(plan.Operation.DestructiveScope));
		}
		for (const MaintenanceCleanTarget& target : plan.CleanTargets)
		{
			model.JobOutput.push_back("Clean target: " + target.Path.string() + " | " + target.Detail);
		}
		for (const std::string& message : plan.ReadinessMessages)
		{
			model.JobOutput.push_back("Readiness: " + message);
		}
		for (const std::string& effect : plan.PlannedEffects)
		{
			model.JobOutput.push_back("Effect: " + effect);
		}
		model.JobOutput.push_back(plan.Operation.DryRunText);
		RecordLauncherShellActivity(model, plan.Operation.DisplayName + " dry-run planned");
	}

	void AppendLaunchPlanDryRun(LauncherShellModel& model, const LaunchOperationPlan& plan)
	{
		model.JobOutput.push_back(plan.Operation.DisplayName + " [" + std::string(plan.CanRun ? "Ready" : "Blocked") + "]");
		model.JobOutput.push_back("Profile: " + plan.Profile);
		model.JobOutput.push_back("Target: " + plan.TargetName);
		model.JobOutput.push_back("Executable: " + plan.ExecutablePath.string());
		model.JobOutput.push_back("Working directory: " + plan.WorkingDirectory.string());
		model.JobOutput.push_back("Latest log: " + plan.Operation.LogPath.string());
		for (const std::string& message : plan.ReadinessMessages)
		{
			model.JobOutput.push_back("Readiness: " + message);
		}
		for (const std::string& effect : plan.PlannedEffects)
		{
			model.JobOutput.push_back("Effect: " + effect);
		}
		model.JobOutput.push_back(plan.Operation.DryRunText);
		RecordLauncherShellActivity(model, plan.Operation.DisplayName + " dry-run planned");
	}

	void ApplyLauncherShellDryRun(LauncherShellModel& model, const LauncherShellArguments& arguments)
	{
		const LauncherShellOperationRow* operation = FindLauncherShellOperation(model, arguments.DryRunOperationId);
		if (operation == nullptr)
		{
			model.JobOutput.push_back("Unknown dry-run operation: " + arguments.DryRunOperationId);
			RecordLauncherShellActivity(model, "Dry-run failed: unknown operation " + arguments.DryRunOperationId);
			return;
		}

		if (FindBuildWorkspaceOperationDefinition(operation->Id).has_value())
		{
			AppendBuildPlanDryRun(model, PlanBuildWorkspaceOperation(operation->Id, BuildWorkspaceShellRequest(model)));
			return;
		}

		if (FindCookOperationDefinition(operation->Id).has_value())
		{
			AppendCookPlanDryRun(model, PlanCookOperation(operation->Id, BuildCookShellRequest(model, arguments)));
			return;
		}

		if (FindLevelOperationDefinition(operation->Id).has_value())
		{
			AppendLevelPlanDryRun(model, PlanLevelOperation(operation->Id, BuildLevelShellRequest(model)));
			return;
		}

		if (FindMaintenanceOperationDefinition(operation->Id).has_value())
		{
			AppendMaintenancePlanDryRun(model, PlanMaintenanceOperation(operation->Id, BuildMaintenanceShellRequest(model, arguments)));
			return;
		}

		if (FindLaunchOperationDefinition(operation->Id).has_value())
		{
			AppendLaunchPlanDryRun(model, PlanLaunchOperation(operation->Id, BuildLaunchShellRequest(model, arguments)));
			return;
		}

		OperationRecord record = MakeOperationRecord(operation->Id, operation->DisplayName);
		record.DryRunText = "Dry-run only: " + operation->NextEffect;
		model.JobOutput.push_back(record.DisplayName + " [Preview]");
		model.JobOutput.push_back(record.DryRunText);
		RecordLauncherShellActivity(model, record.DisplayName + " dry-run");
	}

	int ReportLauncherShellOperationResult(const OperationRecord& operation, std::ostream& output, std::ostream& error)
	{
		output << "Operation " << operation.DisplayName << " finished with status " << ToString(operation.Status) << ".\n";
		if (!operation.LogPath.empty())
		{
			output << "Latest log: " << operation.LogPath.string() << '\n';
		}
		if (!operation.FailureSummary.empty())
		{
			error << operation.FailureSummary << '\n';
		}
		return operation.Status == OperationStatus::Succeeded ? 0 : 1;
	}

	int RunLauncherShellOperation(
	    LauncherShellModel& model,
	    const LauncherShellArguments& arguments,
	    std::ostream& output,
	    std::ostream& error)
	{
		NativeProcessRunner processRunner;
		if (FindBuildWorkspaceOperationDefinition(arguments.RunOperationId).has_value())
		{
			const OperationRecord operation = RunBuildWorkspaceOperationPlan(
			    PlanBuildWorkspaceOperation(arguments.RunOperationId, BuildWorkspaceShellRequest(model)),
			    processRunner,
			    [&output](std::string_view text) { output << text; });
			return ReportLauncherShellOperationResult(operation, output, error);
		}

		if (FindCookOperationDefinition(arguments.RunOperationId).has_value())
		{
			const OperationRecord operation = RunCookOperationPlan(
			    PlanCookOperation(arguments.RunOperationId, BuildCookShellRequest(model, arguments)),
			    processRunner,
			    [&output](std::string_view text) { output << text; });
			return ReportLauncherShellOperationResult(operation, output, error);
		}

		if (FindLevelOperationDefinition(arguments.RunOperationId).has_value())
		{
			const OperationRecord operation = RunLevelOperationPlan(
			    PlanLevelOperation(arguments.RunOperationId, BuildLevelShellRequest(model)),
			    processRunner,
			    [&output](std::string_view text) { output << text; });
			return ReportLauncherShellOperationResult(operation, output, error);
		}

		if (FindMaintenanceOperationDefinition(arguments.RunOperationId).has_value())
		{
			const OperationRecord operation = RunMaintenanceOperationPlan(
			    PlanMaintenanceOperation(arguments.RunOperationId, BuildMaintenanceShellRequest(model, arguments)),
			    processRunner,
			    [&output](std::string_view text) { output << text; });
			return ReportLauncherShellOperationResult(operation, output, error);
		}

		if (FindLaunchOperationDefinition(arguments.RunOperationId).has_value())
		{
			const OperationRecord operation = RunLaunchOperationPlan(
			    PlanLaunchOperation(arguments.RunOperationId, BuildLaunchShellRequest(model, arguments)),
			    processRunner,
			    [&output](std::string_view text) { output << text; });
			return ReportLauncherShellOperationResult(operation, output, error);
		}

		error << "SparkleLauncher: unknown --run operation: " << arguments.RunOperationId << '\n';
		return 1;
	}
}
