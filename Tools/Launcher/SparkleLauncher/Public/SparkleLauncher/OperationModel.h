#pragma once

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace SparkleLauncher
{
	enum class LauncherOperationCategory : std::uint8_t
	{
		Workspace,
		Levels,
		LevelRun,
		Cooking,
		Maintenance
	};

	enum class OperationStatus : std::uint8_t
	{
		Pending,
		Running,
		Succeeded,
		Failed,
		Skipped,
		Canceled
	};

	enum class OperationDestructiveScope : std::uint8_t
	{
		None,
		CookedOutputs,
		BuildTree,
		ArtifactOutputs,
		WorkspaceState,
		DependencyCache,
		Logs,
		PristineGeneratedWorkspace
	};

	struct OperationInput
	{
		std::string Name;
		std::string Value;
	};

	struct OperationRecord
	{
		std::string Id;
		std::string DisplayName;
		std::vector<OperationInput> Inputs;
		OperationStatus Status = OperationStatus::Pending;
		std::filesystem::path LogPath;
		std::chrono::system_clock::time_point StartTime = {};
		std::chrono::system_clock::time_point EndTime = {};
		std::optional<int> ExitCode;
		std::string DryRunText;
		OperationDestructiveScope DestructiveScope = OperationDestructiveScope::None;
		bool RequiresConfirmation = false;
		std::string FailureSummary;
	};

	OperationRecord MakeOperationRecord(std::string id, std::string displayName);
	void MarkOperationStarted(OperationRecord& operation, std::filesystem::path logPath = {});
	void MarkOperationFinished(OperationRecord& operation, OperationStatus status, std::optional<int> exitCode = std::nullopt);
	std::string ToString(OperationStatus status);
	std::string ToString(OperationDestructiveScope scope);
}
