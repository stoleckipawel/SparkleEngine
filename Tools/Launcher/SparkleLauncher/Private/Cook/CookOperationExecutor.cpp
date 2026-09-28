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
			return result.FailureReason + " Log: " + step.Request.LogPath.string();
		}

		if (static_cast<unsigned int>(result.ExitCode) == kMissingRuntimeDependencyExitCode)
		{
			if (step.Id == "cook-shaders")
			{
				return "ShaderCompiler could not start because its DXC/Slang runtime support bundle is incomplete. Rebuild cooking tools "
				       "after "
				       "Sync confirms the Vulkan SDK, then retry. Log: "
				    + step.Request.LogPath.string();
			}
			return "A required runtime DLL is missing for this tool. Rebuild the tool and retry. Log: " + step.Request.LogPath.string();
		}

		if (step.Id == "cook-shaders")
		{
			return "Shader cooking failed. Log: " + step.Request.LogPath.string();
		}
		if (step.Id == "cook-textures")
		{
			return "Texture cooking failed. Log: " + step.Request.LogPath.string();
		}
		if (step.Id == "cook-scene-assets")
		{
			return "Scene, mesh, or material asset cooking failed. Log: " + step.Request.LogPath.string();
		}
		return step.DisplayName + " failed. Log: " + step.Request.LogPath.string();
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
			operation.FailureSummary =
			    plan.ReadinessMessages.empty() ? "Cook operation is not ready to run." : plan.ReadinessMessages.front();
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
					operation.FailureSummary = errorMessage;
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
				operation.ProcessStartFailure = result.StartFailure;
				operation.FailureSummary = MakeCookFailureSummary(step, result);
				MarkOperationFinished(operation, result.Canceled ? OperationStatus::Canceled : OperationStatus::Failed, result.ExitCode);
				return operation;
			}
			ReportOperationProgress(outputCallback, step.DisplayName, stepIndex + 1, processSteps.size());
		}

		MarkOperationFinished(operation, OperationStatus::Succeeded, 0);
		return operation;
	}
}
