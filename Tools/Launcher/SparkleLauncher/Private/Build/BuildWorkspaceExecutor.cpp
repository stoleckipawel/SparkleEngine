#include "SparkleLauncher/BuildWorkspaceOperations.h"

#include "BuildWorkspaceProcessRequests.h"

#include <array>
#include <optional>
#include <system_error>

namespace SparkleLauncher
{
	static bool ClearStaleConfigureState(const BuildWorkspaceOperationPlan& plan, std::string& errorMessage)
	{
		std::error_code errorCode;

		const std::array<std::filesystem::path, 4> generatedFiles = {
		    plan.Freshness.CachePath,
		    plan.Freshness.SolutionPath,
		    plan.Freshness.StampPath,
		    plan.Freshness.BuildDirectory / "CMakeFiles",
		};

		for (const std::filesystem::path& path : generatedFiles)
		{
			errorCode.clear();
			if (!std::filesystem::exists(path, errorCode) || errorCode)
			{
				errorCode.clear();
				continue;
			}

			std::filesystem::remove_all(path, errorCode);
			if (errorCode)
			{
				errorMessage = "Failed to clear stale generated build state: " + path.string();
				return false;
			}
		}

		return true;
	}

	OperationRecord RunBuildWorkspaceOperationPlan(BuildWorkspaceOperationPlan plan, IProcessRunner& processRunner, ProcessOutputCallback outputCallback)
	{
		OperationRecord operation = plan.Operation;
		MarkOperationStarted(operation, operation.LogPath);

		if (!plan.CanRun)
		{
			operation.FailureSummary = plan.ReadinessMessages.empty() ? "Operation is not ready to run." : plan.ReadinessMessages.front();
			MarkOperationFinished(operation, OperationStatus::Failed, std::nullopt);
			return operation;
		}

		for (BuildWorkspaceProcessStep& step : BuildProcessStepsForPlan(plan))
		{
			ProcessRequest request = step.Request;
			if (step.Id == "configure")
			{
				std::error_code errorCode;
				std::filesystem::create_directories(request.WorkingDirectory, errorCode);
				if (plan.Freshness.State == BuildFilesFreshnessState::GeneratorMismatch ||
				    plan.Freshness.State == BuildFilesFreshnessState::FreshnessStampMismatch)
				{
					std::string cleanupError;
					if (!ClearStaleConfigureState(plan, cleanupError))
					{
						operation.FailureSummary = cleanupError;
						MarkOperationFinished(operation, OperationStatus::Failed, std::nullopt);
						return operation;
					}
				}
			}

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

			if (step.UpdatesBuildFilesFreshness)
			{
				std::string errorMessage;
				if (!UpdateBuildFilesFreshnessStamp(plan.RepositoryRoot, plan.Toolchain, errorMessage))
				{
					operation.FailureSummary = errorMessage;
					MarkOperationFinished(operation, OperationStatus::Failed, std::nullopt);
					return operation;
				}
			}
		}

		MarkOperationFinished(operation, OperationStatus::Succeeded, 0);
		return operation;
	}
}
