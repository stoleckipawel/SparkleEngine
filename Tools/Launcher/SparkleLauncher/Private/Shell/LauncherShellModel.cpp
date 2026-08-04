#include "LauncherShellModel.h"

#include "LauncherShellArguments.h"

#include "SparkleLauncher/CookOperations.h"
#include "SparkleLauncher/LaunchOperations.h"
#include "SparkleLauncher/LauncherPaths.h"
#include "SparkleLauncher/MaintenanceOperations.h"

#include <algorithm>
#include <chrono>
#include <ctime>
#include <filesystem>
#include <iomanip>
#include <optional>
#include <sstream>
#include <system_error>
#include <utility>

namespace SparkleLauncher
{
	std::vector<LauncherShellOperationRow> BuildLauncherShellOperationRows()
	{
		std::vector<LauncherShellOperationRow> rows;
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
	}

	std::string GetLauncherShellTimeText()
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

	std::optional<std::filesystem::path> FindLatestLauncherShellLog(const std::filesystem::path& logsDirectory)
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
			if (entry.is_regular_file(errorCode))
			{
				const std::filesystem::file_time_type writeTime = entry.last_write_time(errorCode);
				if (!errorCode && (!latestLogPath.has_value() || writeTime > latestWriteTime))
				{
					latestLogPath = entry.path();
					latestWriteTime = writeTime;
				}
			}

			errorCode.clear();
			logIterator.increment(errorCode);
			errorCode.clear();
		}

		return latestLogPath;
	}

	void AppendLocalLauncherShellActivity(LauncherShellModel& model)
	{
		const LauncherStatePaths statePaths = GetLauncherStatePaths(model.Repository.RootPath);
		if (std::filesystem::exists(statePaths.ActivityPath))
		{
			model.Activity.push_back({"local", "Activity file: " + statePaths.ActivityPath.string()});
		}
		else
		{
			model.Activity.push_back({"local", "No launcher activity recorded yet."});
			model.Activity.push_back({"local", "Launcher state: " + statePaths.RootDirectory.string()});
		}

		const std::optional<std::filesystem::path> latestLogPath = FindLatestLauncherShellLog(statePaths.LogsDirectory);
		model.Activity.push_back(
		    {"local", latestLogPath.has_value() ? "Latest launcher log: " + latestLogPath->string() : "No launcher logs discovered yet."});
	}

	LauncherShellModel BuildLauncherShellModel(RepositoryRoot repository, SparkleContent content, const LauncherShellArguments& arguments)
	{
		LauncherShellModel model;
		model.Repository = std::move(repository);
		model.ContentId = std::move(content.Id);
		model.EditorProfile = arguments.EditorProfile;
		model.RuntimeProfile = arguments.RuntimeProfile;
		model.WorkspaceIdePreference = arguments.WorkspaceIdePreference;
		model.WorkspaceCompilerPreference = arguments.WorkspaceCompilerPreference;
		model.Operations = BuildLauncherShellOperationRows();
		AppendLocalLauncherShellActivity(model);
		return model;
	}

	const LauncherShellOperationRow* FindLauncherShellOperation(const LauncherShellModel& model, std::string_view operationId) noexcept
	{
		const auto found = std::find_if(
		    model.Operations.begin(),
		    model.Operations.end(),
		    [operationId](const LauncherShellOperationRow& operation) { return operation.Id == operationId; });
		return found == model.Operations.end() ? nullptr : &*found;
	}

	void RecordLauncherShellActivity(LauncherShellModel& model, std::string summary)
	{
		model.Activity.push_back({GetLauncherShellTimeText(), std::move(summary)});
	}
}
