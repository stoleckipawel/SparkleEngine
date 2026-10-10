#include "SparkleLauncher/LevelRunOperations.h"

#include "LevelRunOperationProcessRequests.h"
#include "LauncherOperationProgress.h"

#include <optional>

namespace SparkleLauncher
{
	OperationRecord RunLevelRunOperationPlan(LevelRunOperationPlan plan, IProcessRunner& processRunner, const ProcessOutputCallback& outputCallback)
	{
		OperationRecord operation = plan.Operation;
		MarkOperationStarted(operation, operation.LogPath);
		if (!plan.CanRun)
		{
			SetOperationFailure(
			    operation,
			    OperationProblemKind::Prerequisite,
			    plan.ReadinessMessages.empty() ? "Level run is not ready." : plan.ReadinessMessages.front(),
			    "Create the missing executable or cooked output named above, then retry Open.");

			MarkOperationFinished(operation, OperationStatus::Failed, std::nullopt);
			return operation;
		}

		std::vector<LevelRunOperationProcessStep> processSteps = BuildLevelRunProcessStepsForPlan(plan);
		for (std::size_t stepIndex = 0; stepIndex < processSteps.size(); ++stepIndex)
		{
			LevelRunOperationProcessStep& step = processSteps[stepIndex];
			ReportOperationProgress(outputCallback, step.DisplayName);
			ProcessRequest request = step.Request;
			AppendProcessOutputCallback(request, outputCallback);

			const ProcessResult result = processRunner.Run(request);
			if (!result.Launched || result.Canceled || !result.Ready)
			{
				if (result.Canceled)
				{
					SetOperationFailure(operation, OperationProblemKind::Cancellation, step.DisplayName + " was canceled.", "Open the level again when ready.");
				}
				else
				{
					SetProcessOperationFailure(
					    operation,
					    result.StartFailure,
					    result.FailureReason.empty() ? step.DisplayName + " failed." : result.FailureReason,
					    "Review the application startup log, correct the first startup or readiness failure, then retry this level.");
				}
				MarkOperationFinished(operation, result.Canceled ? OperationStatus::Canceled : OperationStatus::Failed, result.ExitCode);
				return operation;
			}
		}

		MarkOperationFinished(operation, OperationStatus::Succeeded, 0);
		return operation;
	}
}
