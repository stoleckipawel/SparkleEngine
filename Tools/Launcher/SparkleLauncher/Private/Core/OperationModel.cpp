#include "SparkleLauncher/OperationModel.h"

#include <utility>

namespace SparkleLauncher
{
	static bool RequiresFailure(OperationStatus status)
	{
		return status == OperationStatus::Failed || status == OperationStatus::Skipped || status == OperationStatus::Canceled;
	}

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
		operation.Failure.reset();
	}

	void MarkOperationFinished(OperationRecord& operation, OperationStatus status, std::optional<int> exitCode)
	{
		if (RequiresFailure(status) && !operation.Failure.has_value())
		{
			SetOperationFailure(
			    operation,
			    OperationProblemKind::Internal,
			    "The operation ended without a structured failure report.",
			    "Retry once; if the problem repeats, report this as a Launcher defect.");
		}
		operation.Status = status;
		operation.EndTime = std::chrono::system_clock::now();
		operation.ExitCode = exitCode;
	}

	void SetOperationFailure(OperationRecord& operation, OperationProblemKind kind, std::string summary, std::string expectedAction)
	{
		operation.Failure = OperationFailure{.Kind = kind, .Summary = std::move(summary), .ExpectedAction = std::move(expectedAction)};
	}

	void SetProcessOperationFailure(
	    OperationRecord& operation,
	    Process::ChildProcessStartFailure startFailure,
	    std::string summary,
	    std::string retryAction)
	{
		switch (startFailure)
		{
			case Process::ChildProcessStartFailure::BlockedByPolicy:
				SetOperationFailure(
				    operation,
				    OperationProblemKind::ProcessStart,
				    std::move(summary),
				    "Use a Sparkle tool bundle signed by a publisher trusted by this machine, or ask the policy administrator to "
				    "authorize that publisher. Rebuilding or retrying does not change the trust decision.");
				return;
			case Process::ChildProcessStartFailure::AccessDenied:
				SetOperationFailure(
				    operation,
				    OperationProblemKind::ProcessStart,
				    std::move(summary),
				    "Check the executable's file permissions and security-product quarantine, then retry.");
				return;
			case Process::ChildProcessStartFailure::ExecutableNotFound:
				SetOperationFailure(
				    operation,
				    OperationProblemKind::ProcessStart,
				    std::move(summary),
				    "Rebuild the owning target, then retry.");
				return;
			case Process::ChildProcessStartFailure::OperatingSystemError:
				SetOperationFailure(
				    operation,
				    OperationProblemKind::ProcessStart,
				    std::move(summary),
				    "Review the operating-system error and the executable path in the operation log, correct that condition, then retry.");
				return;
			case Process::ChildProcessStartFailure::None:
				break;
		}

		SetOperationFailure(operation, OperationProblemKind::ToolExecution, std::move(summary), std::move(retryAction));
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
			case OperationDestructiveScope::CookedOutputs:
				return "CookedOutputs";
			case OperationDestructiveScope::BuildTree:
				return "BuildTree";
			case OperationDestructiveScope::ArtifactOutputs:
				return "ArtifactOutputs";
			case OperationDestructiveScope::WorkspaceState:
				return "WorkspaceState";
			case OperationDestructiveScope::DependencyCache:
				return "DependencyCache";
			case OperationDestructiveScope::Logs:
				return "Logs";
			case OperationDestructiveScope::PristineGeneratedWorkspace:
				return "PristineGeneratedWorkspace";
		}

		return "Unknown";
	}

	std::string ToString(OperationProblemKind kind)
	{
		switch (kind)
		{
			case OperationProblemKind::None:
				return "None";
			case OperationProblemKind::Prerequisite:
				return "Prerequisite";
			case OperationProblemKind::Planning:
				return "Planning";
			case OperationProblemKind::ProcessStart:
				return "Process start";
			case OperationProblemKind::ToolExecution:
				return "Tool execution";
			case OperationProblemKind::OutputValidation:
				return "Expected output missing";
			case OperationProblemKind::Filesystem:
				return "File system";
			case OperationProblemKind::Internal:
				return "Launcher internal";
			case OperationProblemKind::Cancellation:
				return "Cancellation";
		}

		return "Unknown";
	}
}
