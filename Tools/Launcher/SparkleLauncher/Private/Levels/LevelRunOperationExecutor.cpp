#include "SparkleLauncher/LevelRunOperations.h"

#include "LevelRunOperationProcessRequests.h"

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

		for (LevelRunOperationProcessStep& step : BuildLevelRunProcessStepsForPlan(plan))
		{
			ProcessRequest request = step.Request;
			const ProcessOutputCallback existingCallback = request.OutputCallback;
			request.OutputCallback = [existingCallback, outputCallback](std::string_view output)
			{
				if (existingCallback)
				{
					existingCallback(output);
				}
				if (outputCallback)
				{
					outputCallback(output);
				}
			};

			const ProcessResult result = processRunner.Run(request);
			if (!result.Launched || result.Canceled || result.ExitCode != 0)
			{
				operation.FailureSummary = result.FailureReason.empty() ? step.DisplayName + " failed." : result.FailureReason;
				MarkOperationFinished(operation, result.Canceled ? OperationStatus::Canceled : OperationStatus::Failed, result.ExitCode);
				return operation;
			}
		}

		MarkOperationFinished(operation, OperationStatus::Succeeded, 0);
		return operation;
	}
}
