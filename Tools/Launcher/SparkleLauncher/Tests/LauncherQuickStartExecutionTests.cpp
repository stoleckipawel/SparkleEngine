#include "LauncherQuickStartExecution.h"

#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>

namespace SparkleLauncher::LauncherQuickStartExecutionTests
{
	void Require(bool condition, std::string_view message)
	{
		if (!condition)
		{
			throw std::runtime_error(std::string(message));
		}
	}

	LauncherCapabilityResolution OperationStep(
	    const QString& operationId,
	    std::string capabilityId,
	    bool completesGoal = false,
	    std::set<std::string> invalidatedCapabilityIds = {})
	{
		LauncherCapabilityResolution resolution;
		resolution.Result = LauncherCapabilityResolution::Kind::RunOperation;
		resolution.CapabilityId = std::move(capabilityId);
		resolution.OperationRequest = LauncherOperationRequest{.OperationId = operationId};
		resolution.InvalidatedCapabilityIds = std::move(invalidatedCapabilityIds);
		resolution.CompletesGoal = completesGoal;
		return resolution;
	}

	void SuccessfulStepsAdvanceInvalidationState()
	{
		LauncherOperationRequest goalRequest;
		goalRequest.OperationId = "launch.editor";
		LauncherQuickStartExecution execution(goalRequest);
		const LauncherCapabilityResolution syncStep = OperationStep("levels.sync", "content.selected-levels", false, {"content.cooked"});

		Require(execution.BeginOperation("run-1", syncStep).empty(), "A valid Quick Start step was rejected.");
		Require(execution.ActiveRunId() == "run-1", "The active run identity was not retained.");
		Require(
		    execution.CompleteOperation("run-1", "levels.sync", true) == LauncherQuickStartCompletion::Continue,
		    "A successful prerequisite did not continue the workflow.");
		Require(execution.ActiveRunId().isEmpty(), "A completed prerequisite remained active.");
		Require(execution.InvalidatedCapabilityIds().contains("content.cooked"), "Downstream invalidation was not committed.");

		LauncherCapabilityResolution cookStep = OperationStep("cook.all", "content.cooked");
		cookStep.RevalidatedCapabilityIds.insert("content.cooked");
		Require(execution.BeginOperation("run-2", cookStep).empty(), "A step with revalidated capabilities was rejected.");
		Require(
		    !execution.InvalidatedCapabilityIds().contains("content.cooked"),
		    "A graph-revalidated capability remained invalidated in the execution state.");
	}

	void RepeatedSuccessfulStepIsRejectedAsStalled()
	{
		LauncherOperationRequest goalRequest;
		goalRequest.OperationId = "launch.runtime";
		LauncherQuickStartExecution execution(goalRequest);
		const LauncherCapabilityResolution buildStep = OperationStep("workspace.build.runtime", "product.runtime");

		Require(execution.BeginOperation("run-1", buildStep).empty(), "The initial build step was rejected.");
		Require(
		    execution.CompleteOperation("run-1", "workspace.build.runtime", true) == LauncherQuickStartCompletion::Continue,
		    "The initial build step did not complete.");
		Require(
		    execution.BeginOperation("run-2", buildStep).find("did not become ready") != std::string::npos,
		    "A stalled repeated capability operation was accepted.");
	}

	void CompletionMustMatchTheActiveRunAndOperation()
	{
		LauncherOperationRequest goalRequest;
		goalRequest.OperationId = "launch.editor";
		LauncherQuickStartExecution execution(goalRequest);
		const LauncherCapabilityResolution launchStep = OperationStep("launch.editor", "launch.editor", true);

		Require(execution.BeginOperation("run-1", launchStep).empty(), "The launch step was rejected.");
		Require(
		    execution.CompleteOperation("another-run", "launch.editor", true) == LauncherQuickStartCompletion::Ignored,
		    "Completion from another run mutated Quick Start.");
		Require(
		    execution.CompleteOperation("run-1", "another-operation", true) == LauncherQuickStartCompletion::Ignored,
		    "Completion from another operation mutated Quick Start.");
		Require(
		    execution.CompleteOperation("run-1", "launch.editor", true) == LauncherQuickStartCompletion::Completed,
		    "The matching terminal launch did not complete Quick Start.");
	}

	void FailureSettlesTheActiveStep()
	{
		LauncherOperationRequest goalRequest;
		goalRequest.OperationId = "launch.editor";
		LauncherQuickStartExecution execution(goalRequest);
		const LauncherCapabilityResolution cookStep = OperationStep("cook.all", "content.cooked");

		Require(execution.BeginOperation("run-1", cookStep).empty(), "The cook step was rejected.");
		Require(
		    execution.CompleteOperation("run-1", "cook.all", false) == LauncherQuickStartCompletion::Failed,
		    "A failed operation did not fail Quick Start.");
		Require(execution.ActiveRunId().isEmpty(), "A failed operation remained active.");
	}

	using TestFunction = void (*)();

	int Run(std::string_view name, TestFunction test)
	{
		try
		{
			test();
			std::cout << "[PASS] " << name << '\n';
			return 0;
		}
		catch (const std::exception& error)
		{
			std::cerr << "[FAIL] " << name << ": " << error.what() << '\n';
			return 1;
		}
	}
}

int main()
{
	using namespace SparkleLauncher::LauncherQuickStartExecutionTests;
	int failureCount = 0;
	failureCount += Run("successful progression", SuccessfulStepsAdvanceInvalidationState);
	failureCount += Run("stalled step rejection", RepeatedSuccessfulStepIsRejectedAsStalled);
	failureCount += Run("completion identity", CompletionMustMatchTheActiveRunAndOperation);
	failureCount += Run("failure settlement", FailureSettlesTheActiveStep);
	return failureCount == 0 ? 0 : 1;
}
