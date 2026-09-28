#include "SparkleLauncher/LevelRunOperations.h"

#include "LevelRunOperationProcessRequests.h"
#include "LauncherOperationProgress.h"

#include <optional>

namespace SparkleLauncher
{
	OperationRecord RunLevelRunOperationPlan(
	    LevelRunOperationPlan plan,
	    IProcessRunner& processRunner,
	    const ProcessOutputCallback& outputCallback)
	{
		OperationRecord operation = plan.Operation;
		MarkOperationStarted(operation, operation.LogPath);
		if (!plan.CanRun)
		{
			operation.FailureSummary = plan.ReadinessMessages.empty() ? "Level run is not ready." : plan.ReadinessMessages.front();
			MarkOperationFinished(operation, OperationStatus::Failed, std::nullopt);
			return operation;
		}

		std::vector<LevelRunOperationProcessStep> processSteps = BuildLevelRunProcessStepsForPlan(plan);
		for (std::size_t stepIndex = 0; stepIndex < processSteps.size(); ++stepIndex)
		{
			LevelRunOperationProcessStep& step = processSteps[stepIndex];
			ReportOperationProgress(outputCallback, step.DisplayName, stepIndex, processSteps.size());
			ProcessRequest request = step.Request;
			AppendProcessOutputCallback(request, outputCallback);

			const ProcessResult result = processRunner.Run(request);
			if (!result.Launched || result.Canceled || result.ExitCode != 0)
			{
				operation.ProcessStartFailure = result.StartFailure;
				operation.FailureSummary = result.FailureReason.empty() ? step.DisplayName + " failed." : result.FailureReason;
				MarkOperationFinished(operation, result.Canceled ? OperationStatus::Canceled : OperationStatus::Failed, result.ExitCode);
				return operation;
			}
			ReportOperationProgress(outputCallback, step.DisplayName, stepIndex + 1, processSteps.size());
		}

		MarkOperationFinished(operation, OperationStatus::Succeeded, 0);
		return operation;
	}
}
