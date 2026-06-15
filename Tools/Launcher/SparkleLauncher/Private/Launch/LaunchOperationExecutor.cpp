#include "SparkleLauncher/LaunchOperations.h"

#include "LaunchOperationProcessRequests.h"
#include "Smoke/RhiSmokeLaunchOperations.h"
#include "Smoke/RhiSmokePtlasArticleArtifactValidation.h"
#include "Smoke/RhiSmokeParityArtifactValidation.h"
#include "Smoke/RhiSmokePtlasBenchmarkArtifactValidation.h"
#include "Smoke/RhiSmokeTestCatalog.h"

#include <filesystem>
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

		if (IsRhiParitySmokeLaunchOperation(plan.Kind))
		{
			std::error_code errorCode;
			std::filesystem::remove_all(GetRhiSmokeParityArtifactDirectory(plan), errorCode);
			if (errorCode)
			{
				operation.FailureSummary = "Could not clear RHI ray tracing parity artifacts: " + errorCode.message();
				MarkOperationFinished(operation, OperationStatus::Failed, 1);
				return operation;
			}
		}
		if (IsRhiPtlasBenchmarkSmokeLaunchOperation(plan.Kind))
		{
			std::error_code errorCode;
			std::filesystem::remove_all(GetRhiSmokePtlasBenchmarkArtifactDirectory(plan), errorCode);
			if (errorCode)
			{
				operation.FailureSummary = "Could not clear PTLAS benchmark artifacts: " + errorCode.message();
				MarkOperationFinished(operation, OperationStatus::Failed, 1);
				return operation;
			}
		}
		if (IsRhiPtlasArticleSmokeLaunchOperation(plan.Kind))
		{
			std::error_code errorCode;
			std::filesystem::remove_all(GetRhiSmokePtlasArticleArtifactDirectory(plan), errorCode);
			if (errorCode)
			{
				operation.FailureSummary = "Could not clear PTLAS article artifacts: " + errorCode.message();
				MarkOperationFinished(operation, OperationStatus::Failed, 1);
				return operation;
			}
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
		if (IsRhiPtlasBenchmarkSmokeLaunchOperation(plan.Kind))
		{
			std::string failureSummary;
			if (!ValidateRhiSmokePtlasBenchmarkArtifacts(plan, failureSummary))
			{
				operation.FailureSummary = failureSummary.empty() ? "PTLAS benchmark artifact validation failed." : failureSummary;
				MarkOperationFinished(operation, OperationStatus::Failed, 1);
				return operation;
			}
		}
		if (IsRhiPtlasArticleSmokeLaunchOperation(plan.Kind))
		{
			std::string failureSummary;
			if (!ValidateRhiSmokePtlasArticleArtifacts(plan, failureSummary))
			{
				operation.FailureSummary = failureSummary.empty() ? "PTLAS article artifact validation failed." : failureSummary;
				MarkOperationFinished(operation, OperationStatus::Failed, 1);
				return operation;
			}
		}

		MarkOperationFinished(operation, OperationStatus::Succeeded, 0);
		return operation;
	}
}
