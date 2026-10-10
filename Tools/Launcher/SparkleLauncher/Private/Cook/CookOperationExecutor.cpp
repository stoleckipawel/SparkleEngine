#include "SparkleLauncher/CookOperations.h"

#include "CookOperationProcessRequests.h"
#include "LauncherOperationProgress.h"

#include <optional>
#include <system_error>

namespace SparkleLauncher
{
	constexpr unsigned int kMissingRuntimeDependencyExitCode = 0xC0000135u;

	static std::string MakeCookFailureSummary(const CookOperationProcessStep& step, const ProcessResult& result)
	{
		if (!result.FailureReason.empty())
		{
			return result.FailureReason;
		}

		if (static_cast<unsigned int>(result.ExitCode) == kMissingRuntimeDependencyExitCode)
		{
			if (step.Id == "cook-shaders")
			{
				return "ShaderCompiler could not start because its DXC/Slang runtime support bundle is incomplete.";
			}
			return "A required runtime DLL is missing for this cooking tool.";
		}

		if (step.Id == "cook-shaders")
		{
			return "Shader cooking failed.";
		}
		if (step.Id == "cook-textures")
		{
			return "Texture cooking failed.";
		}
		if (step.Id == "cook-scene-assets")
		{
			return "Scene, mesh, or material asset cooking failed.";
		}
		return step.DisplayName + " failed.";
	}

	static std::string CookRecoveryAction(const CookOperationProcessStep& step)
	{
		if (step.Id == "cook-shaders")
		{
			return "Correct the first ShaderCompiler error in the log, rebuild Cooking Tools if shader registration code changed, then "
			       "retry Cook Shaders.";
		}
		if (step.Id == "cook-textures")
		{
			return "Correct the first TextureCooker error in the log, then retry Cook Textures.";
		}
		if (step.Id == "cook-scene-assets")
		{
			return "Correct the first AssetCooker error in the log, then retry Cook Scene Assets.";
		}
		return "Correct the first error in the named log, then retry this cook operation.";
	}

	static bool CleanCookedOutputs(const std::filesystem::path& path, std::string& outErrorMessage)
	{
		std::error_code errorCode;
		if (!std::filesystem::exists(path, errorCode))
		{
			outErrorMessage.clear();
			return true;
		}

		std::filesystem::remove_all(path, errorCode);
		if (errorCode)
		{
			outErrorMessage = "Failed to clean cooked output scope: " + path.string();
			return false;
		}

		outErrorMessage.clear();
		return true;
	}

	OperationRecord RunCookOperationPlan(CookOperationPlan plan, IProcessRunner& processRunner, const ProcessOutputCallback& outputCallback)
	{
		OperationRecord operation = plan.Operation;
		MarkOperationStarted(operation, operation.LogPath);

		if (!plan.CanRun)
		{
			SetOperationFailure(
			    operation,
			    OperationProblemKind::Prerequisite,
			    plan.ReadinessMessages.empty() ? "Cook operation is not ready to run." : plan.ReadinessMessages.front(),
			    "Build the required cooking tools or acquire the missing source content named above, then retry this cook operation.");

			MarkOperationFinished(operation, OperationStatus::Failed, std::nullopt);
			return operation;
		}

		std::vector<CookOperationProcessStep> processSteps = BuildCookProcessStepsForPlan(plan);
		for (std::size_t stepIndex = 0; stepIndex < processSteps.size(); ++stepIndex)
		{
			CookOperationProcessStep& step = processSteps[stepIndex];
			ReportOperationProgress(outputCallback, step.DisplayName, stepIndex, processSteps.size());
			if (step.DeletesCookedOutputs)
			{
				std::string errorMessage;
				if (!CleanCookedOutputs(step.DestructivePath, errorMessage))
				{
					SetOperationFailure(
					    operation,
					    OperationProblemKind::Filesystem,
					    std::move(errorMessage),
					    "Close processes using the cooked output, verify write permission for that path, then retry.");

					MarkOperationFinished(operation, OperationStatus::Failed, std::nullopt);
					return operation;
				}
				ReportOperationProgress(outputCallback, step.DisplayName, stepIndex + 1, processSteps.size());
				continue;
			}

			ProcessRequest request = step.Request;
			if (step.Id == "configure")
			{
				std::error_code errorCode;
				std::filesystem::create_directories(request.WorkingDirectory, errorCode);
			}

			AppendProcessOutputCallback(request, outputCallback);

			const ProcessResult result = processRunner.Run(request);
			if (!result.Launched || result.Canceled || result.ExitCode != 0)
			{
				if (result.Canceled)
				{
					SetOperationFailure(operation, OperationProblemKind::Cancellation, step.DisplayName + " was canceled.", "Run the cook operation again when ready.");
				}
				else
				{
					SetProcessOperationFailure(operation, result.StartFailure, MakeCookFailureSummary(step, result), CookRecoveryAction(step));
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
