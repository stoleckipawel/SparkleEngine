#include "SparkleCli.h"

#include "SparkleLauncher/BuildWorkspaceOperations.h"
#include "SparkleLauncher/LaunchOperations.h"
#include "SparkleLauncher/ProjectDiscovery.h"
#include "SparkleLauncher/RepositoryLocator.h"

#include <algorithm>
#include <filesystem>
#include <iostream>
#include <optional>
#include <string_view>
#include <utility>

namespace SparkleLauncher
{
	static bool TryParseFormatMode(std::string_view text, FormatMode& outMode)
	{
		if (text == "check")
		{
			outMode = FormatMode::Check;
			return true;
		}
		if (text == "apply")
		{
			outMode = FormatMode::Apply;
			return true;
		}
		return false;
	}

	static bool TryParseCleanScope(std::string_view text, CleanScope& outScope)
	{
		if (text == "selected-cooked" || text == "selected-project-cooked-outputs")
		{
			outScope = CleanScope::SelectedProjectCookedOutputs;
			return true;
		}
		if (text == "all-cooked" || text == "all-cooked-outputs")
		{
			outScope = CleanScope::AllCookedOutputs;
			return true;
		}
		if (text == "build-tree")
		{
			outScope = CleanScope::BuildTree;
			return true;
		}
		if (text == "shader-cache")
		{
			outScope = CleanScope::ShaderCache;
			return true;
		}
		if (text == "deps" || text == "third-party-dependency-cache")
		{
			outScope = CleanScope::ThirdPartyDependencyCache;
			return true;
		}
		if (text == "logs")
		{
			outScope = CleanScope::Logs;
			return true;
		}
		if (text == "pristine" || text == "pristine-generated-workspace")
		{
			outScope = CleanScope::PristineGeneratedWorkspace;
			return true;
		}
		return false;
	}

	static std::string ChooseSelectedProjectId(const std::vector<SparkleProject>& projects, std::string_view requestedProjectId)
	{
		if (!requestedProjectId.empty())
		{
			const auto requestedProject = std::find_if(projects.begin(), projects.end(), [requestedProjectId](const SparkleProject& project) {
				return project.Id == requestedProjectId;
			});
			if (requestedProject != projects.end())
			{
				return requestedProject->Id;
			}
			return std::string(requestedProjectId);
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

	static void PrintOperationRecord(const OperationRecord& operation, std::ostream& output)
	{
		output << operation.DisplayName << " [" << ToString(operation.Status) << "]\n";
		output << "Operation: " << operation.Id << "\n";
		if (!operation.LogPath.empty())
		{
			output << "Log: " << operation.LogPath.string() << "\n";
		}
		if (operation.ExitCode.has_value())
		{
			output << "Exit code: " << *operation.ExitCode << "\n";
		}
		if (!operation.FailureSummary.empty())
		{
			output << "Failure: " << operation.FailureSummary << "\n";
		}
	}

	static void PrintPlanDetails(
	    const OperationRecord& operation,
	    bool canRun,
	    const std::vector<std::string>& readinessMessages,
	    const std::vector<std::string>& plannedEffects,
	    std::ostream& output)
	{
		output << operation.DisplayName << " [" << (canRun ? "Ready" : "Blocked") << "]\n";
		output << "Operation: " << operation.Id << "\n";
		if (!operation.LogPath.empty())
		{
			output << "Log: " << operation.LogPath.string() << "\n";
		}
		if (operation.RequiresConfirmation)
		{
			output << "Confirmation required: " << ToString(operation.DestructiveScope) << "\n";
		}
		for (const std::string& message : readinessMessages)
		{
			output << "Readiness: " << message << "\n";
		}
		for (const std::string& effect : plannedEffects)
		{
			output << "Effect: " << effect << "\n";
		}
		if (!operation.DryRunText.empty())
		{
			output << operation.DryRunText << "\n";
		}
	}

	static int FinishDryRun(bool canRun)
	{
		return canRun ? 0 : 2;
	}

	static int FinishOperation(const OperationRecord& operation)
	{
		return operation.Status == OperationStatus::Succeeded ? 0 : 1;
	}

	static void PrintOperationList(std::ostream& output)
	{
		for (const BuildWorkspaceOperationDefinition& definition : GetBuildWorkspaceOperationDefinitions())
		{
			output << definition.Id << " - " << definition.DisplayName << "\n";
		}
		for (const CookOperationDefinition& definition : GetCookOperationDefinitions())
		{
			output << definition.Id << " - " << definition.DisplayName << "\n";
		}
		for (const MaintenanceOperationDefinition& definition : GetMaintenanceOperationDefinitions())
		{
			output << definition.Id << " - " << definition.DisplayName << "\n";
		}
		for (const LaunchOperationDefinition& definition : GetLaunchOperationDefinitions())
		{
			output << definition.Id << " - " << definition.DisplayName << "\n";
		}
	}

	int SparkleCli::Run(int argc, char** argv, std::ostream& output, std::ostream& error) const
	{
		SparkleCliArguments arguments;
		if (!ParseArguments(argc, argv, arguments, error))
		{
			PrintUsage(error);
			return 1;
		}

		if (arguments.ShowHelp)
		{
			PrintUsage(output);
			return 0;
		}

		if (arguments.ListOperations)
		{
			PrintOperationList(output);
			return 0;
		}

		if (arguments.OperationId.empty())
		{
			error << "Sparkle: operation id is required.\n";
			PrintUsage(error);
			return 1;
		}

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

		const std::string projectId = ChooseSelectedProjectId(projects, arguments.ProjectId);
		NativeProcessRunner processRunner;

		if (FindBuildWorkspaceOperationDefinition(arguments.OperationId).has_value())
		{
			BuildWorkspaceOperationRequest request;
			request.RepositoryRoot = repository->RootPath;
			request.ProjectId = projectId;
			request.EditorProfile = arguments.EditorProfile;
			request.RuntimeProfile = arguments.RuntimeProfile;
			request.SelectedTargets = arguments.SelectedTargets;
			request.ForceConfigure = arguments.ForceConfigure;
			BuildWorkspaceOperationPlan plan = PlanBuildWorkspaceOperation(arguments.OperationId, request);
			PrintPlanDetails(plan.Operation, plan.CanRun, plan.ReadinessMessages, plan.PlannedEffects, output);
			if (arguments.DryRun)
			{
				return FinishDryRun(plan.CanRun);
			}
			const OperationRecord operation = RunBuildWorkspaceOperationPlan(std::move(plan), processRunner, [&output](std::string_view line) { output << line; });
			PrintOperationRecord(operation, output);
			return FinishOperation(operation);
		}

		if (FindCookOperationDefinition(arguments.OperationId).has_value())
		{
			CookOperationRequest request;
			request.RepositoryRoot = repository->RootPath;
			request.ProjectId = projectId;
			request.RuntimeProfile = arguments.RuntimeProfile;
			request.Mode = arguments.RequestedCookMode;
			request.ForceRecookConfirmed = arguments.ForceRecookConfirmed;
			request.ShaderPackages = arguments.ShaderPackages;
			CookOperationPlan plan = PlanCookOperation(arguments.OperationId, request);
			PrintPlanDetails(plan.Operation, plan.CanRun, plan.ReadinessMessages, plan.PlannedEffects, output);
			if (arguments.DryRun)
			{
				return FinishDryRun(plan.CanRun);
			}
			const OperationRecord operation = RunCookOperationPlan(std::move(plan), processRunner, [&output](std::string_view line) { output << line; });
			PrintOperationRecord(operation, output);
			return FinishOperation(operation);
		}

		if (FindMaintenanceOperationDefinition(arguments.OperationId).has_value())
		{
			MaintenanceOperationRequest request;
			request.RepositoryRoot = repository->RootPath;
			request.ProjectId = projectId;
			request.EditorProfile = arguments.EditorProfile;
			request.RequestedFormatMode = arguments.RequestedFormatMode;
			request.RequestedCleanScope = arguments.RequestedCleanScope;
			request.DestructiveActionConfirmed = arguments.CleanConfirmed;
			request.ValidationTargets = arguments.ValidationTargets;
			MaintenanceOperationPlan plan = PlanMaintenanceOperation(arguments.OperationId, request);
			PrintPlanDetails(plan.Operation, plan.CanRun, plan.ReadinessMessages, plan.PlannedEffects, output);
			if (arguments.DryRun)
			{
				return FinishDryRun(plan.CanRun);
			}
			const OperationRecord operation = RunMaintenanceOperationPlan(std::move(plan), processRunner, [&output](std::string_view line) { output << line; });
			PrintOperationRecord(operation, output);
			return FinishOperation(operation);
		}

		if (FindLaunchOperationDefinition(arguments.OperationId).has_value())
		{
			LaunchOperationRequest request;
			request.RepositoryRoot = repository->RootPath;
			request.ProjectId = projectId;
			request.EditorProfile = arguments.EditorProfile;
			request.RuntimeProfile = arguments.RuntimeProfile;
			LaunchOperationPlan plan = PlanLaunchOperation(arguments.OperationId, request);
			PrintPlanDetails(plan.Operation, plan.CanRun, plan.ReadinessMessages, plan.PlannedEffects, output);
			if (arguments.DryRun)
			{
				return FinishDryRun(plan.CanRun);
			}
			const OperationRecord operation = RunLaunchOperationPlan(std::move(plan), processRunner, [&output](std::string_view line) { output << line; });
			PrintOperationRecord(operation, output);
			return FinishOperation(operation);
		}

		error << "Sparkle: unknown operation id '" << arguments.OperationId << "'.\n";
		return 1;
	}

	bool SparkleCli::ParseArguments(int argc, char** argv, SparkleCliArguments& outArguments, std::ostream& error) const
	{
		for (int index = 1; index < argc; ++index)
		{
			const std::string_view argument(argv[index]);
			if (argument == "--help" || argument == "-h" || argument == "/?")
			{
				outArguments.ShowHelp = true;
				continue;
			}
			if (argument == "--list-operations")
			{
				outArguments.ListOperations = true;
				continue;
			}
			if (argument == "--dry-run")
			{
				outArguments.DryRun = true;
				continue;
			}
			if (argument == "--force-configure")
			{
				outArguments.ForceConfigure = true;
				continue;
			}
			if (argument == "--force-recook")
			{
				outArguments.RequestedCookMode = CookMode::Force;
				continue;
			}
			if (argument == "--confirm-force-recook")
			{
				outArguments.ForceRecookConfirmed = true;
				continue;
			}
			if (argument == "--confirm-clean")
			{
				outArguments.CleanConfirmed = true;
				continue;
			}

			auto requireValue = [&](std::string_view optionName) -> const char* {
				if (index + 1 >= argc)
				{
					error << "Sparkle: " << optionName << " requires a value.\n";
					return nullptr;
				}
				return argv[++index];
			};

			if (argument == "--root")
			{
				const char* value = requireValue(argument);
				if (value == nullptr)
				{
					return false;
				}
				outArguments.StartPath = value;
				continue;
			}
			if (argument == "--project")
			{
				const char* value = requireValue(argument);
				if (value == nullptr)
				{
					return false;
				}
				outArguments.ProjectId = value;
				continue;
			}
			if (argument == "--editor-profile")
			{
				const char* value = requireValue(argument);
				if (value == nullptr)
				{
					return false;
				}
				outArguments.EditorProfile = value;
				continue;
			}
			if (argument == "--runtime-profile")
			{
				const char* value = requireValue(argument);
				if (value == nullptr)
				{
					return false;
				}
				outArguments.RuntimeProfile = value;
				continue;
			}
			if (argument == "--target")
			{
				const char* value = requireValue(argument);
				if (value == nullptr)
				{
					return false;
				}
				outArguments.SelectedTargets.push_back(value);
				continue;
			}
			if (argument == "--shader-package")
			{
				const char* value = requireValue(argument);
				if (value == nullptr)
				{
					return false;
				}
				outArguments.ShaderPackages.push_back(value);
				continue;
			}
			if (argument == "--validation-target")
			{
				const char* value = requireValue(argument);
				if (value == nullptr)
				{
					return false;
				}
				outArguments.ValidationTargets.push_back(value);
				continue;
			}
			if (argument == "--format-mode")
			{
				const char* value = requireValue(argument);
				FormatMode mode = FormatMode::Check;
				if (value == nullptr || !TryParseFormatMode(value, mode))
				{
					error << "Sparkle: --format-mode expects check or apply.\n";
					return false;
				}
				outArguments.RequestedFormatMode = mode;
				continue;
			}
			if (argument == "--clean-scope")
			{
				const char* value = requireValue(argument);
				CleanScope scope = CleanScope::SelectedProjectCookedOutputs;
				if (value == nullptr || !TryParseCleanScope(value, scope))
				{
					error << "Sparkle: unsupported clean scope.\n";
					return false;
				}
				outArguments.RequestedCleanScope = scope;
				continue;
			}

			if (!argument.empty() && argument.front() == '-')
			{
				error << "Sparkle: unexpected argument '" << argument << "'.\n";
				return false;
			}
			if (!outArguments.OperationId.empty())
			{
				error << "Sparkle: multiple operation ids were provided.\n";
				return false;
			}
			outArguments.OperationId = argument;
		}

		return true;
	}

	void SparkleCli::PrintUsage(std::ostream& output) const
	{
		output << "Usage:\n"
		       << "  Sparkle <operation-id> [--dry-run] [--root <repo-root>] [--project <project-id>] [--editor-profile <profile>] [--runtime-profile <profile>]\n"
		       << "  Sparkle --list-operations\n"
		       << "\n"
		       << "Options:\n"
		       << "  --target <target>                 Add an explicit build target.\n"
		       << "  --force-configure                 Force CMake configure before build operations.\n"
		       << "  --force-recook                    Use force recook mode for cook operations.\n"
		       << "  --confirm-force-recook            Confirm force recook destructive cleanup.\n"
		       << "  --shader-package <package-id>     Add a focused shader package cook target.\n"
		       << "  --format-mode check|apply         Select clang-format mode.\n"
		       << "  --validation-target <target>      Add a CMake validation target.\n"
		       << "  --clean-scope <scope>             selected-cooked, all-cooked, build-tree, shader-cache, deps, logs, pristine.\n"
		       << "  --confirm-clean                   Confirm destructive clean scope.\n"
		       << "\n"
		       << "Examples:\n"
		       << "  Sparkle workspace.setup --dry-run\n"
		       << "  Sparkle project.build.editor --project Showcase --editor-profile DevelopmentEditor\n"
		       << "  Sparkle cook.shaders --project Showcase --runtime-profile DevelopmentGame --dry-run\n"
		       << "  Sparkle workspace.clean --clean-scope selected-cooked --confirm-clean --dry-run\n";
	}
}