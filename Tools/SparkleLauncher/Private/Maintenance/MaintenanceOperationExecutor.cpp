#include "SparkleLauncher/MaintenanceOperations.h"

#include "MaintenanceOperationProcessRequests.h"

#include <optional>
#include <system_error>

namespace SparkleLauncher
{
	static bool RemovePath(const std::filesystem::path& path, std::string& outErrorMessage)
	{
		std::error_code errorCode;
		if (!std::filesystem::exists(path, errorCode))
		{
			outErrorMessage.clear();
			return true;
		}

		if (std::filesystem::is_directory(path, errorCode))
		{
			std::filesystem::remove_all(path, errorCode);
		}
		else
		{
			std::filesystem::remove(path, errorCode);
		}

		if (errorCode || std::filesystem::exists(path, errorCode))
		{
			outErrorMessage = "Clean blocked by locked files or permissions: " + path.string();
			return false;
		}

		outErrorMessage.clear();
		return true;
	}

	static bool RemoveBuildDirectoryContentsPreservingDependencies(const std::filesystem::path& buildDirectory, std::string& outErrorMessage)
	{
		std::error_code errorCode;
		if (!std::filesystem::is_directory(buildDirectory, errorCode))
		{
			outErrorMessage.clear();
			return true;
		}

		for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(buildDirectory, errorCode))
		{
			if (entry.path().filename() == "_deps")
			{
				continue;
			}
			if (!RemovePath(entry.path(), outErrorMessage))
			{
				return false;
			}
		}

		outErrorMessage.clear();
		return true;
	}

	static bool IsRootGeneratedFile(const std::filesystem::path& path)
	{
		const std::string extension = path.extension().string();
		const std::string filename = path.filename().string();
		return extension == ".sln" || extension == ".slnx" || extension == ".vcxproj" || extension == ".filters" || extension == ".user" ||
		    filename == "CMakeCache.txt" || filename == "cmake_install.cmake" || filename == "Makefile";
	}

	static bool RemoveRootGeneratedFiles(const std::filesystem::path& repositoryRoot, std::string& outErrorMessage)
	{
		std::error_code errorCode;
		for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(repositoryRoot, errorCode))
		{
			if (entry.is_directory(errorCode) && entry.path().filename() == "CMakeFiles")
			{
				if (!RemovePath(entry.path(), outErrorMessage))
				{
					return false;
				}
				continue;
			}

			if (entry.is_regular_file(errorCode) && IsRootGeneratedFile(entry.path()))
			{
				if (!RemovePath(entry.path(), outErrorMessage))
				{
					return false;
				}
			}
		}

		outErrorMessage.clear();
		return true;
	}

	static bool RunCleanStep(const MaintenanceOperationProcessStep& step, std::string& outErrorMessage)
	{
		switch (step.CleanBehavior)
		{
		case MaintenanceCleanBehavior::RemovePath:
			return RemovePath(step.DestructivePath, outErrorMessage);
		case MaintenanceCleanBehavior::RemoveBuildDirectoryContentsPreservingDependencies:
			return RemoveBuildDirectoryContentsPreservingDependencies(step.DestructivePath, outErrorMessage);
		case MaintenanceCleanBehavior::RemoveRootGeneratedFiles:
			return RemoveRootGeneratedFiles(step.DestructivePath, outErrorMessage);
		}

		outErrorMessage = "Unknown clean behavior.";
		return false;
	}

	static std::string MakeMaintenanceFailureSummary(const MaintenanceOperationProcessStep& step, const ProcessResult& result)
	{
		if (!result.FailureReason.empty())
		{
			return result.FailureReason + " Log: " + step.Request.LogPath.string();
		}
		if (step.Id == "clang-format-batch")
		{
			return "clang-format reported formatting differences or failed. Log: " + step.Request.LogPath.string();
		}
		if (step.Id == "validation-gates")
		{
			return "Validation gates failed. Log: " + step.Request.LogPath.string();
		}
		return step.DisplayName + " failed. Log: " + step.Request.LogPath.string();
	}

	static std::string MakeCleanFailureSummary(const MaintenanceOperationProcessStep& step, const OperationRecord& operation, const std::string& errorMessage)
	{
		std::string summary = errorMessage.empty() ? "Clean blocked by locked files or permissions." : errorMessage;
		summary += " Scope: " + step.DestructivePath.string();
		if (!operation.LogPath.empty())
		{
			summary += " Latest log: " + operation.LogPath.string();
		}
		return summary;
	}

	OperationRecord RunMaintenanceOperationPlan(MaintenanceOperationPlan plan, IProcessRunner& processRunner, ProcessOutputCallback outputCallback)
	{
		OperationRecord operation = plan.Operation;
		MarkOperationStarted(operation, operation.LogPath);

		if (!plan.CanRun)
		{
			operation.FailureSummary = plan.ReadinessMessages.empty() ? "Maintenance operation is not ready to run." : plan.ReadinessMessages.front();
			MarkOperationFinished(operation, OperationStatus::Failed, std::nullopt);
			return operation;
		}

		for (MaintenanceOperationProcessStep& step : BuildMaintenanceProcessStepsForPlan(plan))
		{
			if (step.DeletesGeneratedOutput)
			{
				std::string errorMessage;
				if (!RunCleanStep(step, errorMessage))
				{
					operation.FailureSummary = MakeCleanFailureSummary(step, operation, errorMessage);
					MarkOperationFinished(operation, OperationStatus::Failed, std::nullopt);
					return operation;
				}
				continue;
			}

			ProcessRequest request = step.Request;
			if (step.Id == "configure")
			{
				std::error_code errorCode;
				std::filesystem::create_directories(request.WorkingDirectory, errorCode);
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
				operation.FailureSummary = MakeMaintenanceFailureSummary(step, result);
				MarkOperationFinished(operation, result.Canceled ? OperationStatus::Canceled : OperationStatus::Failed, result.ExitCode);
				return operation;
			}
		}

		MarkOperationFinished(operation, OperationStatus::Succeeded, 0);
		return operation;
	}
}