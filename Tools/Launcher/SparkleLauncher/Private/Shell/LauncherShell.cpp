#include "LauncherShell.h"

#include "SparkleLauncher/BuildWorkspaceOperations.h"
#include "SparkleLauncher/LauncherProjectDefaults.h"
#include "SparkleLauncher/LauncherPaths.h"

#include <algorithm>
#include <chrono>
#include <ctime>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <optional>
#include <sstream>
#include <string_view>
#include <system_error>

namespace SparkleLauncher
{
	struct LauncherOperationRow
	{
		std::string Group;
		std::string Id;
		std::string DisplayName;
		std::string Readiness;
		std::string NextEffect;
	};

	struct LauncherActivityEntry
	{
		std::string TimeText;
		std::string Summary;
	};

	struct LauncherShellState
	{
		RepositoryRoot Repository;
		std::vector<SparkleProject> Projects;
		std::string SelectedProjectId;
		std::string EditorProfile = "DevelopmentEditor";
		std::string RuntimeProfile = "DevelopmentGame";
		WorkspaceIde WorkspaceIdePreference = WorkspaceIde::VisualStudio;
		std::vector<LauncherOperationRow> Operations;
		std::vector<LauncherActivityEntry> Activity;
		std::vector<std::string> JobOutput;
	};

	static constexpr std::string_view kDefaultDryRunOperationId = "workspace.sync-source-tiers";

	static const std::vector<LauncherOperationRow>& GetLauncherOperationRows()
	{
		static const std::vector<LauncherOperationRow> operations = [] {
			std::vector<LauncherOperationRow> rows;
			for (const BuildWorkspaceOperationDefinition& definition : GetBuildWorkspaceOperationDefinitions())
			{
				rows.push_back({definition.Group, definition.Id, definition.DisplayName, "Dry-run", definition.Description});
			}
			for (const CookOperationDefinition& definition : GetCookOperationDefinitions())
			{
				rows.push_back({definition.Group, definition.Id, definition.DisplayName, "Dry-run", definition.Description});
			}
			for (const MaintenanceOperationDefinition& definition : GetMaintenanceOperationDefinitions())
			{
				rows.push_back({definition.Group, definition.Id, definition.DisplayName, "Dry-run", definition.Description});
			}
			for (const LaunchOperationDefinition& definition : GetLaunchOperationDefinitions())
			{
				rows.push_back({definition.Group, definition.Id, definition.DisplayName, "Dry-run", definition.Description});
			}
			return rows;
		}();
		return operations;
	}

	static std::string GetCurrentTimeText()
	{
		const auto now = std::chrono::system_clock::now();
		const std::time_t time = std::chrono::system_clock::to_time_t(now);
		std::tm localTime = {};
#if defined(_WIN32)
		localtime_s(&localTime, &time);
#else
		localtime_r(&time, &localTime);
#endif

		std::ostringstream stream;
		stream << std::put_time(&localTime, "%H:%M");
		return stream.str();
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
		}

		const auto defaultProject = std::find_if(projects.begin(), projects.end(), [](const SparkleProject& project) {
			return project.Id == kDefaultProjectId;
		});
		if (defaultProject != projects.end())
		{
			return defaultProject->Id;
		}

		return projects.empty() ? std::string() : projects.front().Id;
	}

	static const LauncherOperationRow* FindOperationRow(const std::vector<LauncherOperationRow>& operations, std::string_view operationId)
	{
		const auto found = std::find_if(operations.begin(), operations.end(), [operationId](const LauncherOperationRow& operation) {
			return operation.Id == operationId;
		});
		return found == operations.end() ? nullptr : &*found;
	}

	static std::optional<std::filesystem::path> FindLatestLauncherLog(const std::filesystem::path& logsDirectory)
	{
		std::error_code errorCode;
		if (!std::filesystem::exists(logsDirectory, errorCode) || !std::filesystem::is_directory(logsDirectory, errorCode))
		{
			return std::nullopt;
		}

		std::optional<std::filesystem::path> latestLogPath;
		std::filesystem::file_time_type latestWriteTime = {};
		std::filesystem::recursive_directory_iterator logIterator(
		    logsDirectory,
		    std::filesystem::directory_options::skip_permission_denied,
		    errorCode);
		if (errorCode)
		{
			return std::nullopt;
		}

		const std::filesystem::recursive_directory_iterator endIterator;
		while (logIterator != endIterator)
		{
			const std::filesystem::directory_entry entry = *logIterator;
			if (!entry.is_regular_file(errorCode))
			{
				errorCode.clear();
				logIterator.increment(errorCode);
				errorCode.clear();
				continue;
			}

			const std::filesystem::file_time_type writeTime = entry.last_write_time(errorCode);
			if (errorCode)
			{
				errorCode.clear();
				logIterator.increment(errorCode);
				errorCode.clear();
				continue;
			}

			if (!latestLogPath.has_value() || writeTime > latestWriteTime)
			{
				latestLogPath = entry.path();
				latestWriteTime = writeTime;
			}

			logIterator.increment(errorCode);
			errorCode.clear();
		}

		return latestLogPath;
	}

	static bool IsProfileTarget(std::string_view profileName, BuildProfileTarget target)
	{
		const std::optional<BuildProfile> profile = FindBuildProfile(profileName);
		return profile.has_value() && profile->Target == target;
	}

	static std::string BuildProfileOptionText(BuildProfileTarget target)
	{
		std::string text;
		for (const BuildProfile& profile : GetBuildProfileCatalog())
		{
			if (profile.Target != target)
			{
				continue;
			}

			if (!text.empty())
			{
				text += ", ";
			}
			text += profile.Name;
		}
		return text;
	}

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

	static void AppendLocalActivity(LauncherShellState& state)
	{
		const LauncherStatePaths statePaths = GetLauncherStatePaths(state.Repository.RootPath);
		const std::filesystem::path activityPath = statePaths.ActivityPath;
		if (std::filesystem::exists(activityPath))
		{
			state.Activity.push_back({"local", "Activity file: " + activityPath.string()});
		}
		else
		{
			state.Activity.push_back({"local", "No launcher activity recorded yet."});
			state.Activity.push_back({"local", "Launcher state: " + statePaths.RootDirectory.string()});
		}

		const std::optional<std::filesystem::path> latestLogPath = FindLatestLauncherLog(statePaths.LogsDirectory);
		state.Activity.push_back(
		    {"local", latestLogPath.has_value() ? "Latest launcher log: " + latestLogPath->string() : "No launcher logs discovered yet."});
	}

	static void AppendBuildPlanDryRun(LauncherShellState& state, const BuildWorkspaceOperationPlan& plan)
	{
		state.JobOutput.push_back(plan.Operation.DisplayName + " [" + std::string(plan.CanRun ? "Ready" : "Blocked") + "]");
		state.JobOutput.push_back("Project: " + state.SelectedProjectId);
		state.JobOutput.push_back("Editor profile: " + state.EditorProfile);
		state.JobOutput.push_back("Runtime profile: " + state.RuntimeProfile);
		state.JobOutput.push_back("Workspace IDE: " + DisplayName(state.WorkspaceIdePreference));
		state.JobOutput.push_back("Latest log: " + plan.Operation.LogPath.string());
		for (const std::string& message : plan.ReadinessMessages)
		{
			state.JobOutput.push_back("Readiness: " + message);
		}
		for (const std::string& effect : plan.PlannedEffects)
		{
			state.JobOutput.push_back("Effect: " + effect);
		}
		state.JobOutput.push_back(plan.Operation.DryRunText);
		state.Activity.push_back({GetCurrentTimeText(), plan.Operation.DisplayName + " dry-run planned for " + state.SelectedProjectId});
	}

	static void AppendCookPlanDryRun(LauncherShellState& state, const CookOperationPlan& plan)
	{
		state.JobOutput.push_back(plan.Operation.DisplayName + " [" + std::string(plan.CanRun ? "Ready" : "Blocked") + "]");
		state.JobOutput.push_back("Project: " + state.SelectedProjectId);
		state.JobOutput.push_back("Runtime profile: " + state.RuntimeProfile);
		state.JobOutput.push_back("Cook mode: " + ToString(plan.Request.Mode));
		state.JobOutput.push_back("Cooked output: " + plan.CookedOutputDirectory.string());
		state.JobOutput.push_back("Latest log: " + plan.Operation.LogPath.string());
		if (plan.Operation.RequiresConfirmation)
		{
			state.JobOutput.push_back("Confirmation required: " + ToString(plan.Operation.DestructiveScope));
		}
		for (const std::string& message : plan.ReadinessMessages)
		{
			state.JobOutput.push_back("Readiness: " + message);
		}
		for (const std::string& effect : plan.PlannedEffects)
		{
			state.JobOutput.push_back("Effect: " + effect);
		}
		state.JobOutput.push_back(plan.Operation.DryRunText);
		state.Activity.push_back({GetCurrentTimeText(), plan.Operation.DisplayName + " dry-run planned for " + state.SelectedProjectId});
	}

	static void AppendMaintenancePlanDryRun(LauncherShellState& state, const MaintenanceOperationPlan& plan)
	{
		state.JobOutput.push_back(plan.Operation.DisplayName + " [" + std::string(plan.CanRun ? "Ready" : "Blocked") + "]");
		state.JobOutput.push_back("Project: " + state.SelectedProjectId);
		state.JobOutput.push_back("Editor profile: " + state.EditorProfile);
		state.JobOutput.push_back("Format mode: " + ToString(plan.Request.RequestedFormatMode));
		state.JobOutput.push_back("Clean scope: " + ToString(plan.Request.RequestedCleanScope));
		state.JobOutput.push_back("Latest log: " + plan.Operation.LogPath.string());
		if (plan.Operation.RequiresConfirmation)
		{
			state.JobOutput.push_back("Confirmation required: " + ToString(plan.Operation.DestructiveScope));
		}
		for (const MaintenanceCleanTarget& target : plan.CleanTargets)
		{
			state.JobOutput.push_back("Clean target: " + target.Path.string() + " | " + target.Detail);
		}
		for (const std::string& message : plan.ReadinessMessages)
		{
			state.JobOutput.push_back("Readiness: " + message);
		}
		for (const std::string& effect : plan.PlannedEffects)
		{
			state.JobOutput.push_back("Effect: " + effect);
		}
		state.JobOutput.push_back(plan.Operation.DryRunText);
		state.Activity.push_back({GetCurrentTimeText(), plan.Operation.DisplayName + " dry-run planned for " + state.SelectedProjectId});
	}

	static void AppendLaunchPlanDryRun(LauncherShellState& state, const LaunchOperationPlan& plan)
	{
		state.JobOutput.push_back(plan.Operation.DisplayName + " [" + std::string(plan.CanRun ? "Ready" : "Blocked") + "]");
		state.JobOutput.push_back("Project: " + state.SelectedProjectId);
		state.JobOutput.push_back("Profile: " + plan.Profile);
		state.JobOutput.push_back("Target: " + plan.TargetName);
		state.JobOutput.push_back("Executable: " + plan.ExecutablePath.string());
		state.JobOutput.push_back("Working directory: " + plan.WorkingDirectory.string());
		state.JobOutput.push_back("Latest log: " + plan.Operation.LogPath.string());
		for (const std::string& message : plan.ReadinessMessages)
		{
			state.JobOutput.push_back("Readiness: " + message);
		}
		for (const std::string& effect : plan.PlannedEffects)
		{
			state.JobOutput.push_back("Effect: " + effect);
		}
		state.JobOutput.push_back(plan.Operation.DryRunText);
		state.Activity.push_back({GetCurrentTimeText(), plan.Operation.DisplayName + " dry-run planned for " + state.SelectedProjectId});
	}

	static void ApplyDryRun(LauncherShellState& state, const LauncherShellArguments& arguments)
	{
		const std::string operationId = arguments.DryRunOperationId.empty() ? std::string(kDefaultDryRunOperationId) : arguments.DryRunOperationId;
		const LauncherOperationRow* operation = FindOperationRow(state.Operations, operationId);
		if (operation == nullptr)
		{
			state.JobOutput.push_back("Unknown dry-run operation: " + operationId);
			state.Activity.push_back({GetCurrentTimeText(), "Dry-run failed: unknown operation " + operationId});
			return;
		}

		BuildWorkspaceOperationRequest buildRequest;
		buildRequest.RepositoryRoot = state.Repository.RootPath;
		buildRequest.ProjectId = state.SelectedProjectId;
		buildRequest.EditorProfile = state.EditorProfile;
		buildRequest.RuntimeProfile = state.RuntimeProfile;
		buildRequest.PreferredIde = state.WorkspaceIdePreference;
		if (FindBuildWorkspaceOperationDefinition(operationId).has_value())
		{
			AppendBuildPlanDryRun(state, PlanBuildWorkspaceOperation(operationId, buildRequest));
			return;
		}

		CookOperationRequest cookRequest;
		cookRequest.RepositoryRoot = state.Repository.RootPath;
		cookRequest.ProjectId = state.SelectedProjectId;
		cookRequest.RuntimeProfile = state.RuntimeProfile;
		cookRequest.Mode = arguments.RequestedCookMode;
		cookRequest.ForceRecookConfirmed = arguments.ForceRecookConfirmed;
		if (FindCookOperationDefinition(operationId).has_value())
		{
			AppendCookPlanDryRun(state, PlanCookOperation(operationId, cookRequest));
			return;
		}

		MaintenanceOperationRequest maintenanceRequest;
		maintenanceRequest.RepositoryRoot = state.Repository.RootPath;
		maintenanceRequest.ProjectId = state.SelectedProjectId;
		maintenanceRequest.EditorProfile = state.EditorProfile;
		maintenanceRequest.RequestedFormatMode = arguments.RequestedFormatMode;
		maintenanceRequest.RequestedCleanScope = arguments.RequestedCleanScope;
		maintenanceRequest.DestructiveActionConfirmed = arguments.CleanConfirmed;
		if (FindMaintenanceOperationDefinition(operationId).has_value())
		{
			AppendMaintenancePlanDryRun(state, PlanMaintenanceOperation(operationId, maintenanceRequest));
			return;
		}

		LaunchOperationRequest launchRequest;
		launchRequest.RepositoryRoot = state.Repository.RootPath;
		launchRequest.ProjectId = state.SelectedProjectId;
		launchRequest.EditorProfile = state.EditorProfile;
		launchRequest.RuntimeProfile = state.RuntimeProfile;
		launchRequest.Target = arguments.LaunchTarget;
		launchRequest.StartupLevel = arguments.LaunchStartupLevel;
		launchRequest.EnableSmokeTest = arguments.EnableSmokeTest;
		launchRequest.SmokeBackend = arguments.SmokeBackend;
		launchRequest.SmokeFrameLimit = arguments.SmokeFrameLimit;
		launchRequest.SmokeViewMode = arguments.SmokeViewMode;
		launchRequest.SmokeCapturePath = arguments.SmokeCapturePath;
		launchRequest.SmokeTrace = arguments.SmokeTrace;
		launchRequest.SmokeSkipLevelSwitching = arguments.SmokeSkipLevelSwitching;
		if (FindLaunchOperationDefinition(operationId).has_value())
		{
			AppendLaunchPlanDryRun(state, PlanLaunchOperation(operationId, launchRequest));
			return;
		}

		OperationRecord record = MakeOperationRecord(operation->Id, operation->DisplayName);
		record.DryRunText = "Dry-run only: " + operation->NextEffect;
		state.JobOutput.push_back(record.DisplayName + " [Preview]");
		state.JobOutput.push_back(record.DryRunText);
		state.Activity.push_back({GetCurrentTimeText(), record.DisplayName + " dry-run for " + state.SelectedProjectId});
	}

	static void RenderProjectTiles(const LauncherShellState& state, std::ostream& output)
	{
		output << "Projects\n";
		if (state.Projects.empty())
		{
			output << "  [Blocked] No .sparkle-project markers were discovered.\n";
			return;
		}

		for (const SparkleProject& project : state.Projects)
		{
			const bool selected = project.Id == state.SelectedProjectId;
			output << "  [" << (selected ? "Selected" : "Ready") << "] " << project.DisplayName
			       << " | " << state.EditorProfile << " | marker: " << project.MarkerPath.string() << '\n';
		}
	}

	static void RenderOperationGroup(const LauncherShellState& state, std::string_view groupName, std::ostream& output)
	{
		output << groupName << "\n";
		for (const LauncherOperationRow& operation : state.Operations)
		{
			if (operation.Group != groupName)
			{
				continue;
			}

			output << "  " << operation.DisplayName << " [" << operation.Readiness << "] " << operation.NextEffect << '\n';
		}
	}

	static void RenderLauncherShell(const LauncherShellState& state, std::ostream& output)
	{
		output << "Sparkle Launcher\n";
		output << "Repository: " << state.Repository.RootPath.string() << "\n\n";
		RenderProjectTiles(state, output);
		output << "\nSelected project: " << (state.SelectedProjectId.empty() ? "<none>" : state.SelectedProjectId) << '\n';
		output << "Profile selectors\n";
		output << "  Editor: " << state.EditorProfile << " | options: " << BuildProfileOptionText(BuildProfileTarget::Editor) << '\n';
		output << "  Runtime/Cook: " << state.RuntimeProfile << " | options: " << BuildProfileOptionText(BuildProfileTarget::Game) << '\n';
		output << "  Workspace IDE: " << DisplayName(state.WorkspaceIdePreference) << " | options: Visual Studio, Rider\n\n";

		RenderOperationGroup(state, "Launch", output);
		RenderOperationGroup(state, "Sync", output);
		RenderOperationGroup(state, "Build", output);
		RenderOperationGroup(state, "Cook", output);
		RenderOperationGroup(state, "Test", output);
		RenderOperationGroup(state, "Package", output);
		RenderOperationGroup(state, "Maintain", output);

		output << "\nRecent Activity\n";
		for (const LauncherActivityEntry& entry : state.Activity)
		{
			output << "  " << entry.TimeText << "  " << entry.Summary << '\n';
		}

		output << "\nJob Output\n";
		if (state.JobOutput.empty())
		{
			output << "  No active job. Use --dry-run [operation-id] to preview an operation.\n";
			return;
		}

		for (const std::string& line : state.JobOutput)
		{
			output << "  " << line << '\n';
		}
	}

	int LauncherShell::Run(int argc, char** argv, std::ostream& output, std::ostream& error) const
	{
		LauncherShellArguments arguments;
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

		std::string errorMessage;
		const std::filesystem::path startPath = arguments.StartPath.empty() ? std::filesystem::current_path() : arguments.StartPath;
		const std::optional<RepositoryRoot> repository = TryFindRepositoryRoot(startPath, errorMessage);
		if (!repository.has_value())
		{
			error << errorMessage << '\n';
			return 1;
		}

		LauncherShellState state;
		state.Repository = *repository;
		state.Projects = DiscoverProjects(state.Repository.RootPath, errorMessage);
		if (!errorMessage.empty())
		{
			error << errorMessage << '\n';
			return 1;
		}

		state.SelectedProjectId = ChooseSelectedProjectId(state.Projects, arguments.SelectedProject);
		state.EditorProfile = arguments.EditorProfile;
		state.RuntimeProfile = arguments.RuntimeProfile;
		state.WorkspaceIdePreference = arguments.WorkspaceIdePreference;
		state.Operations = GetLauncherOperationRows();
		AppendLocalActivity(state);
		if (!arguments.DryRunOperationId.empty())
		{
			ApplyDryRun(state, arguments);
		}

		RenderLauncherShell(state, output);
		return 0;
	}

	bool LauncherShell::ParseArguments(int argc, char** argv, LauncherShellArguments& outArguments, std::ostream& error) const
	{
		for (int index = 1; index < argc; ++index)
		{
			const std::string_view argument(argv[index]);
			if (argument == "--help" || argument == "-h" || argument == "/?")
			{
				outArguments.ShowHelp = true;
				continue;
			}

			if (argument == "--root")
			{
				if (index + 1 >= argc)
				{
					error << "SparkleLauncher: --root requires a path.\n";
					return false;
				}
				outArguments.StartPath = argv[++index];
				continue;
			}

			if (argument == "--project")
			{
				if (index + 1 >= argc)
				{
					error << "SparkleLauncher: --project requires a project id.\n";
					return false;
				}
				outArguments.SelectedProject = argv[++index];
				continue;
			}

			if (argument == "--editor-profile")
			{
				if (index + 1 >= argc)
				{
					error << "SparkleLauncher: --editor-profile requires a profile.\n";
					return false;
				}

				const std::string_view profile(argv[++index]);
				if (!IsProfileTarget(profile, BuildProfileTarget::Editor))
				{
					error << "SparkleLauncher: unsupported editor profile '" << profile << "'.\n";
					return false;
				}
				outArguments.EditorProfile = profile;
				continue;
			}

			if (argument == "--runtime-profile")
			{
				if (index + 1 >= argc)
				{
					error << "SparkleLauncher: --runtime-profile requires a profile.\n";
					return false;
				}

				const std::string_view profile(argv[++index]);
				if (!IsProfileTarget(profile, BuildProfileTarget::Game))
				{
					error << "SparkleLauncher: unsupported runtime profile '" << profile << "'.\n";
					return false;
				}
				outArguments.RuntimeProfile = profile;
				continue;
			}

			if (argument == "--dry-run")
			{
				if (index + 1 < argc && argv[index + 1][0] != '-')
				{
					outArguments.DryRunOperationId = argv[++index];
				}
				else
				{
					outArguments.DryRunOperationId = std::string(kDefaultDryRunOperationId);
				}
				continue;
			}

			if (argument == "--ide")
			{
				if (index + 1 >= argc)
				{
					error << "SparkleLauncher: --ide requires visual-studio or rider.\n";
					return false;
				}

				WorkspaceIde ide = WorkspaceIde::VisualStudio;
				const std::string_view ideText(argv[++index]);
				if (!TryParseWorkspaceIde(ideText, ide))
				{
					error << "SparkleLauncher: unsupported IDE '" << ideText << "'.\n";
					return false;
				}
				outArguments.WorkspaceIdePreference = ide;
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

			if (argument == "--format-mode")
			{
				if (index + 1 >= argc)
				{
					error << "SparkleLauncher: --format-mode requires check or apply.\n";
					return false;
				}

				FormatMode mode = FormatMode::Check;
				const std::string_view modeText(argv[++index]);
				if (!TryParseFormatMode(modeText, mode))
				{
					error << "SparkleLauncher: unsupported format mode '" << modeText << "'.\n";
					return false;
				}
				outArguments.RequestedFormatMode = mode;
				continue;
			}

			if (argument == "--clean-scope")
			{
				if (index + 1 >= argc)
				{
					error << "SparkleLauncher: --clean-scope requires a scope.\n";
					return false;
				}

				CleanScope scope = CleanScope::SelectedProjectCookedOutputs;
				const std::string_view scopeText(argv[++index]);
				if (!TryParseCleanScope(scopeText, scope))
				{
					error << "SparkleLauncher: unsupported clean scope '" << scopeText << "'.\n";
					return false;
				}
				outArguments.RequestedCleanScope = scope;
				continue;
			}

			if (argument == "--confirm-clean")
			{
				outArguments.CleanConfirmed = true;
				continue;
			}

			if (argument == "--smoke-trace")
			{
				outArguments.SmokeTrace = true;
				continue;
			}

			if (argument == "--smoke-skip-level-switching")
			{
				outArguments.SmokeSkipLevelSwitching = true;
				continue;
			}

			if (argument == "--smoke-backend")
			{
				if (index + 1 >= argc)
				{
					error << "SparkleLauncher: --smoke-backend requires a value.\n";
					return false;
				}
				outArguments.SmokeBackend = argv[++index];
				continue;
			}

			if (argument == "--smoke-frame-limit")
			{
				if (index + 1 >= argc)
				{
					error << "SparkleLauncher: --smoke-frame-limit requires a value.\n";
					return false;
				}
				outArguments.SmokeFrameLimit = argv[++index];
				continue;
			}

			if (argument == "--smoke-view-mode")
			{
				if (index + 1 >= argc)
				{
					error << "SparkleLauncher: --smoke-view-mode requires a value.\n";
					return false;
				}
				outArguments.SmokeViewMode = argv[++index];
				continue;
			}

			if (argument == "--smoke-capture")
			{
				if (index + 1 >= argc)
				{
					error << "SparkleLauncher: --smoke-capture requires a path.\n";
					return false;
				}
				outArguments.SmokeCapturePath = argv[++index];
				continue;
			}

			if (argument == "--launch-target")
			{
				if (index + 1 >= argc)
				{
					error << "SparkleLauncher: --launch-target requires a value.\n";
					return false;
				}
				const std::string target = argv[++index];
				if (target != "editor" && target != "runtime")
				{
					error << "SparkleLauncher: --launch-target must be editor or runtime.\n";
					return false;
				}
				outArguments.LaunchTarget = target;
				continue;
			}

			if (argument == "--startup-level")
			{
				if (index + 1 >= argc)
				{
					error << "SparkleLauncher: --startup-level requires a value.\n";
					return false;
				}
				outArguments.LaunchStartupLevel = argv[++index];
				continue;
			}

			if (argument == "--smoke-test")
			{
				outArguments.EnableSmokeTest = true;
				continue;
			}

			error << "SparkleLauncher: unexpected argument '" << argument << "'.\n";
			return false;
		}

		return true;
	}

	void LauncherShell::PrintUsage(std::ostream& output) const
	{
		output << "Usage:\n"
		       << "  SparkleLauncher [--root <repo-root>] [--project <project-id>] [--editor-profile <profile>] [--runtime-profile <profile>] [--ide <visual-studio|rider>] [--launch-target <editor|runtime>] [--startup-level <level-name>] [--smoke-test] [--format-mode check|apply] [--clean-scope <scope>] [--confirm-clean] [--force-recook] [--confirm-force-recook] [--smoke-backend <backend>] [--smoke-frame-limit <frames>] [--smoke-view-mode <index>] [--smoke-capture <path>] [--smoke-trace] [--smoke-skip-level-switching] [--dry-run [operation-id]]\n"
		       << "\n"
		       << "Examples:\n"
		       << "  SparkleLauncher --dry-run\n"
		       << "  SparkleLauncher --project " << kDefaultProjectId << " --runtime-profile DevelopmentGame --dry-run cook.shaders\n"
		       << "  SparkleLauncher --project " << kDefaultProjectId << " --launch-target runtime --startup-level Sponza --smoke-test --smoke-backend d3d12 --smoke-view-mode 3 --smoke-capture logs/smoke/scene-color.bmp --dry-run project.run\n"
		       << "  SparkleLauncher --project " << kDefaultProjectId << " --force-recook --dry-run cook.project\n"
		       << "  SparkleLauncher --format-mode check --dry-run quality.format\n"
		       << "  SparkleLauncher --clean-scope selected-cooked --dry-run workspace.clean\n";
	}
}
