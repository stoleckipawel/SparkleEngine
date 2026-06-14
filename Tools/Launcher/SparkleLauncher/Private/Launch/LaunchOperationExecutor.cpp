#include "SparkleLauncher/LaunchOperations.h"

#include "LaunchOperationProcessRequests.h"
#include "Smoke/RhiSmokeLaunchOperations.h"
#include "Smoke/RhiSmokeParityArtifactValidation.h"

#include <optional>

namespace SparkleLauncher
{
	OperationRecord RunLaunchOperationPlan(LaunchOperationPlan plan, IProcessRunner& processRunner, ProcessOutputCallback outputCallback)
	{
		OperationRecord operation = plan.Operation;
		MarkOperationStarted(operation, operation.LogPath);

		if (!plan.CanRun)
		{
			operation.FailureSummary = plan.ReadinessMessages.empty() ? "Launch operation is not ready to run." : plan.ReadinessMessages.front();
			MarkOperationFinished(operation, OperationStatus::Failed, std::nullopt);
			return operation;
		}

		for (LaunchOperationProcessStep& step : BuildLaunchProcessStepsForPlan(plan))
		{
			ProcessRequest request = step.Request;
			const ProcessOutputCallback existingCallback = request.OutputCallback;
			request.OutputCallback = [existingCallback, outputCallback](std::string_view output) {
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

		if (IsRhiParitySmokeLaunchOperation(plan.Kind))
		{
			std::string failureSummary;
			if (!ValidateRhiSmokeRayTracingParityArtifacts(plan, failureSummary))
			{
				operation.FailureSummary = failureSummary.empty() ? "RHI ray tracing parity artifact validation failed." : failureSummary;
				MarkOperationFinished(operation, OperationStatus::Failed, 1);
				return operation;
			}
		}

		MarkOperationFinished(operation, OperationStatus::Succeeded, 0);
		return operation;
	}
}
