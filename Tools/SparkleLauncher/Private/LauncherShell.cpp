#include "LauncherShell.h"

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
		std::vector<LauncherOperationRow> Operations;
		std::vector<LauncherActivityEntry> Activity;
		std::vector<std::string> JobOutput;
	};

	static constexpr std::string_view kDefaultDryRunOperationId = "workspace.setup";

	static const std::vector<LauncherOperationRow>& GetLauncherOperationRows()
	{
		static const std::vector<LauncherOperationRow> operations = {
		    {"Setup", "workspace.setup", "Setup Workspace", "Ready", "Validate toolchain and ensure build files."},
		    {"Setup", "workspace.generate-solution", "Generate Solution", "Ready", "Refresh CMake build files."},
		    {"Setup", "toolchain.check", "Check Toolchain", "Ready", "Inspect required local tools."},
		    {"Build", "project.build.editor", "Compile Editor", "Ready", "Build <Project>Editor for the selected editor profile."},
		    {"Build", "project.build.runtime", "Compile Runtime", "Ready", "Build <Project>Runtime for the selected runtime profile."},
		    {"Build", "cook.tools.prepare", "Build Cook Tools", "Ready", "Prepare AssetCooker, TextureCooker, and ShaderCompiler."},
		    {"Cook", "cook.project", "Cook All Assets", "Dry-run", "Preview full incremental cook for the selected project."},
		    {"Cook", "cook.shaders", "Cook Shaders", "Dry-run", "Preview shader package cook."},
		    {"Cook", "cook.textures", "Build Textures", "Dry-run", "Preview focused texture cook."},
		    {"Cook", "cook.assets", "Build Meshes / Scene Assets", "Dry-run", "Preview scene, mesh, and material cook."},
		    {"Maintenance", "workspace.clean", "Clean Workspace", "Needs scope", "Require an explicit generated-output scope."},
		    {"Maintenance", "quality.format", "Run Clang Format", "Dry-run", "Preview source formatting pass."},
		    {"Maintenance", "quality.validate", "Run Validation Gates", "Dry-run", "Preview CMake validation target list."},
		    {"Launch", "project.launch.editor", "Run Editor", "Blocked", "Compile editor target before launch."},
		    {"Launch", "project.launch.runtime", "Run Runtime", "Blocked", "Compile runtime target before launch."},
		};
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

		const auto showcaseProject = std::find_if(projects.begin(), projects.end(), [](const SparkleProject& project) {
			return project.Id == "Showcase";
		});
		if (showcaseProject != projects.end())
		{
			return showcaseProject->Id;
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

	static void ApplyDryRun(LauncherShellState& state, std::string_view requestedOperationId)
	{
		const std::string operationId = requestedOperationId.empty() ? std::string(kDefaultDryRunOperationId) : std::string(requestedOperationId);
		const LauncherOperationRow* operation = FindOperationRow(state.Operations, operationId);
		if (operation == nullptr)
		{
			state.JobOutput.push_back("Unknown dry-run operation: " + operationId);
			state.Activity.push_back({GetCurrentTimeText(), "Dry-run failed: unknown operation " + operationId});
			return;
		}

		OperationRecord record = MakeOperationRecord(operation->Id, operation->DisplayName);
		record.Inputs.push_back({"project", state.SelectedProjectId});
		record.Inputs.push_back({"editorProfile", state.EditorProfile});
		record.Inputs.push_back({"runtimeProfile", state.RuntimeProfile});
		record.DryRunText = "Dry-run only: " + operation->NextEffect;
		MarkOperationStarted(record, GetLauncherOperationLogPath(state.Repository.RootPath, record.Id, "DryRun.txt"));
		MarkOperationFinished(record, OperationStatus::Succeeded, 0);

		state.JobOutput.push_back(record.DisplayName + " [" + ToString(record.Status) + "]");
		state.JobOutput.push_back("Project: " + state.SelectedProjectId);
		state.JobOutput.push_back("Editor profile: " + state.EditorProfile);
		state.JobOutput.push_back("Runtime profile: " + state.RuntimeProfile);
		state.JobOutput.push_back("Log path: " + record.LogPath.string());
		state.JobOutput.push_back(record.DryRunText);
		state.Activity.push_back({GetCurrentTimeText(), record.DisplayName + " dry-run succeeded for " + state.SelectedProjectId});
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
		output << "  Runtime/Cook: " << state.RuntimeProfile << " | options: " << BuildProfileOptionText(BuildProfileTarget::Game) << "\n\n";

		RenderOperationGroup(state, "Setup", output);
		RenderOperationGroup(state, "Build", output);
		RenderOperationGroup(state, "Cook", output);
		RenderOperationGroup(state, "Maintenance", output);
		RenderOperationGroup(state, "Launch", output);

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
		state.Operations = GetLauncherOperationRows();
		AppendLocalActivity(state);
		if (!arguments.DryRunOperationId.empty())
		{
			ApplyDryRun(state, arguments.DryRunOperationId);
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

			error << "SparkleLauncher: unexpected argument '" << argument << "'.\n";
			return false;
		}

		return true;
	}

	void LauncherShell::PrintUsage(std::ostream& output) const
	{
		output << "Usage:\n"
		       << "  SparkleLauncher [--root <repo-root>] [--project <project-id>] [--editor-profile <profile>] [--runtime-profile <profile>] [--dry-run [operation-id]]\n"
		       << "\n"
		       << "Examples:\n"
		       << "  SparkleLauncher --dry-run\n"
		       << "  SparkleLauncher --project Showcase --runtime-profile DevelopmentGame --dry-run cook.shaders\n";
	}
}