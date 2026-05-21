#include "SparkleLauncher/OperationModel.h"

#include <utility>

namespace SparkleLauncher
{
	OperationRecord MakeOperationRecord(std::string id, std::string displayName)
	{
		OperationRecord operation;
		operation.Id = std::move(id);
		operation.DisplayName = std::move(displayName);
		return operation;
	}

	void MarkOperationStarted(OperationRecord& operation, std::filesystem::path logPath)
	{
		operation.Status = OperationStatus::Running;
		operation.StartTime = std::chrono::system_clock::now();
		operation.EndTime = {};
		operation.ExitCode.reset();
		operation.LogPath = std::move(logPath);
		operation.FailureSummary.clear();
	}

	void MarkOperationFinished(OperationRecord& operation, OperationStatus status, std::optional<int> exitCode)
	{
		operation.Status = status;
		operation.EndTime = std::chrono::system_clock::now();
		operation.ExitCode = exitCode;
	}

	std::string ToString(OperationStatus status)
	{
		switch (status)
		{
		case OperationStatus::Pending:
			return "Pending";
		case OperationStatus::Running:
			return "Running";
		case OperationStatus::Succeeded:
			return "Succeeded";
		case OperationStatus::Failed:
			return "Failed";
		case OperationStatus::Skipped:
			return "Skipped";
		case OperationStatus::Canceled:
			return "Canceled";
		}

		return "Unknown";
	}

	std::string ToString(OperationDestructiveScope scope)
	{
		switch (scope)
		{
		case OperationDestructiveScope::None:
			return "None";
		case OperationDestructiveScope::SelectedProjectCookedOutputs:
			return "SelectedProjectCookedOutputs";
		case OperationDestructiveScope::AllCookedOutputs:
			return "AllCookedOutputs";
		case OperationDestructiveScope::BuildTree:
			return "BuildTree";
		case OperationDestructiveScope::DependencyCache:
			return "DependencyCache";
		case OperationDestructiveScope::Logs:
			return "Logs";
		case OperationDestructiveScope::PristineGeneratedWorkspace:
			return "PristineGeneratedWorkspace";
		}

		return "Unknown";
	}
}