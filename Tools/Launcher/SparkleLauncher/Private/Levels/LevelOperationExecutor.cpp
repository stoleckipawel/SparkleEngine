#include "SparkleLauncher/LevelOperations.h"

#include "LevelOperationProcessRequests.h"
#include "LauncherOperationProgress.h"

#include "Core/Public/Diagnostics/Error.h"

#include <algorithm>

namespace SparkleLauncher
{
	static bool MatchesPlannedStep(const LevelOperationStep& planned, const LevelOperationProcessStep& executable)
	{
		return planned.Id == executable.Id && planned.DisplayName == executable.DisplayName
		    && planned.DisplayCommandLine == BuildDisplayCommandLine(executable.Request.ExecutablePath, executable.Request.Arguments) && planned.LogPath == executable.Request.LogPath;
	}

	static std::string MakeLevelOperationFailureSummary(const LevelOperationProcessStep& step, const ProcessResult& result)
	{
		if (!result.FailureReason.empty())
		{
			return result.FailureReason;
		}
		return "Asset pack acquisition failed.";
	}

	bool LevelOperationExecutionPlanMatches(const LevelOperationPlan& plan, const std::vector<LevelOperationProcessStep>& processSteps)
	{
		return plan.Steps.size() == processSteps.size()
		    && std::equal(
		        plan.Steps.begin(),
		        plan.Steps.end(),
		        processSteps.begin(),
		        [](const LevelOperationStep& planned, const LevelOperationProcessStep& executable) { return MatchesPlannedStep(planned, executable); });
	}

	OperationRecord RunLevelOperationPlan(LevelOperationPlan plan, IProcessRunner& processRunner, const ProcessOutputCallback& outputCallback)
	{
		OperationRecord operation = plan.Operation;
		MarkOperationStarted(operation, operation.LogPath);

		if (!plan.CanRun)
		{
			SetOperationFailure(
			    operation,
			    OperationProblemKind::Prerequisite,
			    plan.ReadinessMessages.empty() ? "Level operation is not ready to run." : plan.ReadinessMessages.back(),
			    "Resolve the reported source or content prerequisite, then retry Sync.");

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
			SetOperationFailure(
			    operation,
			    OperationProblemKind::Planning,
			    std::string("Level operation planning failed: ") + error.what(),
			    "Refresh the level workflow, review its preview, then retry.");

			MarkOperationFinished(operation, OperationStatus::Failed, std::nullopt);
			return operation;
		}
		if (!LevelOperationExecutionPlanMatches(plan, processSteps))
		{
			SetOperationFailure(operation, OperationProblemKind::Planning, "Level operation inputs changed after planning.", "Refresh the level workflow, review its updated preview, then retry.");
			MarkOperationFinished(operation, OperationStatus::Failed, std::nullopt);
			return operation;
		}

		for (std::size_t stepIndex = 0; stepIndex < processSteps.size(); ++stepIndex)
		{
			LevelOperationProcessStep& step = processSteps[stepIndex];
			ReportOperationProgress(outputCallback, step.DisplayName, stepIndex, processSteps.size());
			ProcessRequest request = step.Request;
			AppendProcessOutputCallback(request, outputCallback);

			const ProcessResult result = processRunner.Run(request);
			if (!result.Launched || result.Canceled || result.ExitCode != 0)
			{
				if (result.Canceled)
				{
					SetOperationFailure(operation, OperationProblemKind::Cancellation, step.DisplayName + " was canceled.", "Run Sync again when ready.");
				}
				else
				{
					SetProcessOperationFailure(
					    operation,
					    result.StartFailure,
					    MakeLevelOperationFailureSummary(step, result),
					    "Correct the first asset acquisition error in the named log, then retry Sync.");
				}
				MarkOperationFinished(operation, result.Canceled ? OperationStatus::Canceled : OperationStatus::Failed, result.ExitCode);
				return operation;
			}
			ReportOperationProgress(outputCallback, step.DisplayName, stepIndex + 1, processSteps.size());
		}

		MarkOperationFinished(operation, OperationStatus::Succeeded, 0);
		return operation;
	}
}
