#include "SparkleLauncher/LevelOperations.h"

#include "LevelOperationProcessRequests.h"

#include "Core/Public/Diagnostics/Error.h"

#include <algorithm>

namespace SparkleLauncher
{
	static bool MatchesPlannedStep(const LevelOperationStep& planned, const LevelOperationProcessStep& executable)
	{
		return planned.Id == executable.Id && planned.DisplayName == executable.DisplayName
		    && planned.DisplayCommandLine == BuildDisplayCommandLine(executable.Request.ExecutablePath, executable.Request.Arguments)
		    && planned.LogPath == executable.Request.LogPath;
	}

	static std::string MakeLevelOperationFailureSummary(const LevelOperationProcessStep& step, const ProcessResult& result)
	{
		const std::string logSuffix = step.Request.LogPath.empty() ? std::string() : " Log: " + step.Request.LogPath.string();
		if (!result.FailureReason.empty())
		{
			return result.FailureReason + logSuffix;
		}
		return "Asset pack acquisition failed." + logSuffix;
	}

	bool LevelOperationExecutionPlanMatches(const LevelOperationPlan& plan, const std::vector<LevelOperationProcessStep>& processSteps)
	{
		return plan.Steps.size() == processSteps.size()
		    && std::equal(
		        plan.Steps.begin(),
		        plan.Steps.end(),
		        processSteps.begin(),
		        [](const LevelOperationStep& planned, const LevelOperationProcessStep& executable)
		        { return MatchesPlannedStep(planned, executable); });
	}

	OperationRecord RunLevelOperationPlan(
	    LevelOperationPlan plan,
	    IProcessRunner& processRunner,
	    const ProcessOutputCallback& outputCallback)
	{
		OperationRecord operation = plan.Operation;
		MarkOperationStarted(operation, operation.LogPath);

		if (!plan.CanRun)
		{
			operation.FailureSummary =
			    plan.ReadinessMessages.empty() ? "Level operation is not ready to run." : plan.ReadinessMessages.back();
			MarkOperationFinished(operation, OperationStatus::Failed, std::nullopt);
			return operation;
		}

		std::vector<LevelOperationProcessStep> processSteps;
		try
		{
			processSteps = BuildLevelOperationProcessSteps(plan);
		}
		catch (const Diagnostics::Error& error)
		{
			operation.FailureSummary = std::string("Level operation planning failed: ") + error.what();
			MarkOperationFinished(operation, OperationStatus::Failed, std::nullopt);
			return operation;
		}
		if (!LevelOperationExecutionPlanMatches(plan, processSteps))
		{
			operation.FailureSummary = "Level operation inputs changed after planning. Refresh the workflow and run it again.";
			MarkOperationFinished(operation, OperationStatus::Failed, std::nullopt);
			return operation;
		}

		for (LevelOperationProcessStep& step : processSteps)
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
				operation.FailureSummary = MakeLevelOperationFailureSummary(step, result);
				MarkOperationFinished(operation, result.Canceled ? OperationStatus::Canceled : OperationStatus::Failed, result.ExitCode);
				return operation;
			}
		}

		MarkOperationFinished(operation, OperationStatus::Succeeded, 0);
		return operation;
	}
}
