#include "SparkleLauncher/BuildWorkspaceOperations.h"

#include "BuildWorkspaceProcessRequests.h"
#include "Core/Public/Diagnostics/Error.h"
#include "SparkleLauncher/LauncherPaths.h"
#include "SparkleLauncher/SourceDependencyState.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <fstream>
#include <optional>
#include <sstream>
#include <string_view>
#include <system_error>
#include <vector>

namespace SparkleLauncher
{
	static std::string TrimCopy(std::string_view text)
	{
		std::size_t first = 0;
		while (first < text.size() && std::isspace(static_cast<unsigned char>(text[first])))
		{
			++first;
		}

		std::size_t last = text.size();
		while (last > first && std::isspace(static_cast<unsigned char>(text[last - 1])))
		{
			--last;
		}

		return std::string(text.substr(first, last - first));
	}

	static std::string ReadLogText(const std::filesystem::path& logPath)
	{
		if (logPath.empty())
		{
			return {};
		}

		std::ifstream stream(logPath, std::ios::binary);
		if (!stream)
		{
			return {};
		}

		std::ostringstream contents;
		contents << stream.rdbuf();
		return contents.str();
	}

	static std::string ExtractCMakeFailureDetail(std::string_view text, bool prioritizeConfigureFailures)
	{
		if (text.empty())
		{
			return {};
		}

		static constexpr std::string_view prioritizedNeedles[] = {
		    "Failed to download NVIDIA Streamline SDK",
		    "NVIDIA Streamline SDK recovery is incomplete",
		    "NVIDIA Streamline SDK extraction is missing",
		    "SPARKLE_RHI_D3D12_NVAPI_INCLUDE_DIR does not contain nvapi.h",
		    "SPARKLE_RHI_D3D12_NVAPI_LIBRARY does not exist",
		    "SPARKLE_RHI_D3D12_NVAPI is ON, but SPARKLE_RHI_D3D12_NVAPI_INCLUDE_DIR is empty",
		    "SPARKLE_RHI_D3D12_NVAPI is ON, but SPARKLE_RHI_D3D12_NVAPI_LIBRARY is empty",
		    "Build step for sparkle_nvapi failed",
		    "Corrupt or incomplete NVIDIA NVAPI source cache detected",
		    "Failed to get the hash for HEAD",
		    "not a git repository",
		    "source directory is missing",
		    "Error removing directory",
		    "FETCHCONTENT_SOURCE_DIR_",
		    "VULKAN_SDK is not set",
		    "VULKAN_SDK is set to",
		    "dxcapi.h",
		    "dxcompiler.dll",
		    "slang.dll",
		    "slang-compiler.dll",
		    "slang-glsl-module.dll",
		    "slang-glslang.dll",
		    "slang-rt.dll",
		    "slang.slang",
		    "slang-standard-module",
		};

		std::istringstream stream{std::string(text)};
		std::vector<std::string> lines;
		for (std::string line; std::getline(stream, line);)
		{
			lines.push_back(TrimCopy(line));
		}

		if (prioritizeConfigureFailures)
		{
			for (std::string_view needle : prioritizedNeedles)
			{
				const auto found = std::find_if(
				    lines.begin(),
				    lines.end(),
				    [needle](const std::string& line) { return line.find(needle) != std::string::npos; });
				if (found != lines.end())
				{
					return *found;
				}
			}
		}

		for (std::size_t index = 0; index < lines.size(); ++index)
		{
			const std::string& line = lines[index];
			if (!(line.rfind("CMake Error at ", 0) == 0 || line.rfind("CMake Error: ", 0) == 0))
			{
				continue;
			}

			std::ostringstream detail;
			for (std::size_t nextIndex = index + 1; nextIndex < lines.size(); ++nextIndex)
			{
				const std::string& candidate = lines[nextIndex];
				if (candidate.empty())
				{
					if (detail.tellp() > 0)
					{
						break;
					}
					continue;
				}
				if (candidate.rfind("Call Stack", 0) == 0)
				{
					break;
				}
				if (detail.tellp() > 0)
				{
					detail << ' ';
				}
				detail << candidate;
			}

			const std::string flattened = detail.str();
			return flattened.empty() ? line : flattened;
		}

		return {};
	}

	static std::string ExtractFailureDetail(const BuildWorkspaceProcessStep& step, const ProcessResult& result)
	{
		std::string text = result.CapturedOutput;
		if (text.empty())
		{
			text = ReadLogText(step.Request.LogPath);
		}

		if (step.Id == "configure" || step.Id.starts_with("sync-asset-pack-"))
		{
			return ExtractCMakeFailureDetail(text, step.Id == "configure");
		}
		return {};
	}

	static std::string MakeBuildWorkspaceFailureSummary(const BuildWorkspaceProcessStep& step, const ProcessResult& result)
	{
		const std::string logSuffix = step.Request.LogPath.empty() ? std::string() : " Log: " + step.Request.LogPath.string();
		if (!result.FailureReason.empty())
		{
			return result.FailureReason + logSuffix;
		}
		const std::string detail = ExtractFailureDetail(step, result);
		if (step.Id == "configure")
		{
			return detail.empty() ? "Generate project files failed." + logSuffix : "Generate project files failed: " + detail + logSuffix;
		}
		if (step.Id.starts_with("sync-asset-pack-"))
		{
			return detail.empty() ? "Asset pack acquisition failed." + logSuffix : "Asset pack acquisition failed: " + detail + logSuffix;
		}
		if (step.Id == "build")
		{
			return detail.empty() ? "Build targets failed." + logSuffix : "Build targets failed: " + detail + logSuffix;
		}
		if (step.Id == "open-ide")
		{
			return detail.empty() ? "Open IDE failed." + logSuffix : "Open IDE failed: " + detail + logSuffix;
		}
		return detail.empty() ? step.DisplayName + " failed." + logSuffix : step.DisplayName + " failed: " + detail + logSuffix;
	}

	static std::optional<std::string> ValidateEnabledSourceDependenciesAfterConfigure(const BuildWorkspaceOperationPlan& plan)
	{
		if (plan.Kind != BuildWorkspaceOperationKind::SyncSourceTiers && plan.Kind != BuildWorkspaceOperationKind::SyncAll
		    && plan.Kind != BuildWorkspaceOperationKind::GenerateBuildFiles)
		{
			return std::nullopt;
		}

		const SourceDependencyInventoryStatus status = InspectSourceDependencyCache(GetBuildDirectory(plan.RepositoryRoot) / "_deps");
		if (status.AllEnabledDependenciesReady)
		{
			return std::nullopt;
		}

		std::ostringstream summary;
		summary << "Enabled source dependency cache is still incomplete after configure.";
		if (!status.ReadinessMessages.empty())
		{
			summary << ' ' << status.ReadinessMessages.front();
			if (status.ReadinessMessages.size() > 1)
			{
				summary << " +" << (status.ReadinessMessages.size() - 1) << " more issue(s).";
			}
		}
		return summary.str();
	}

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

	static bool ClearSourceDependencyCache(const BuildWorkspaceOperationPlan& plan, std::string& errorMessage)
	{
		const std::filesystem::path dependencyCachePath = GetBuildDirectory(plan.RepositoryRoot) / "_deps";
		std::error_code errorCode;
		if (!std::filesystem::exists(dependencyCachePath, errorCode) || errorCode)
		{
			errorMessage.clear();
			return true;
		}

		std::filesystem::remove_all(dependencyCachePath, errorCode);
		if (errorCode)
		{
			errorMessage = "Failed to clear stale source dependency cache: " + dependencyCachePath.string();
			return false;
		}

		errorMessage.clear();
		return true;
	}

	static bool ShouldRetryConfigureAfterDependencyRecovery(
	    const BuildWorkspaceOperationPlan& plan,
	    const BuildWorkspaceProcessStep& step,
	    const ProcessResult& result)
	{
		if (step.Id != "configure")
		{
			return false;
		}

		if (plan.Kind != BuildWorkspaceOperationKind::SyncSourceTiers && plan.Kind != BuildWorkspaceOperationKind::SyncAll
		    && plan.Kind != BuildWorkspaceOperationKind::GenerateBuildFiles)
		{
			return false;
		}

		std::string text = result.CapturedOutput;
		if (text.empty())
		{
			text = ReadLogText(step.Request.LogPath);
		}
		if (text.empty())
		{
			return false;
		}

		static constexpr std::string_view retryNeedles[] = {
		    "not a git repository",
		    "source directory is missing",
		    "Error removing directory",
		    "FETCHCONTENT_SOURCE_DIR_",
		    "Build step for assimp failed",
		    "Build step for sparkle_nvapi failed",
		    "Corrupt/partial clone detected",
		    "Corrupt or incomplete NVIDIA NVAPI source cache detected",
		};

		return std::any_of(
		    std::begin(retryNeedles),
		    std::end(retryNeedles),
		    [&text](std::string_view needle) { return text.find(needle) != std::string::npos; });
	}

	static bool MatchesPlannedStep(const BuildWorkspaceOperationStep& planned, const BuildWorkspaceProcessStep& executable)
	{
		return planned.Id == executable.Id && planned.DisplayName == executable.DisplayName
		    && planned.DisplayCommandLine == BuildDisplayCommandLine(executable.Request.ExecutablePath, executable.Request.Arguments)
		    && planned.LogPath == executable.Request.LogPath && planned.UpdatesBuildFilesFreshness == executable.UpdatesBuildFilesFreshness;
	}

	bool BuildWorkspaceExecutionPlanMatches(
	    const BuildWorkspaceOperationPlan& plan,
	    const std::vector<BuildWorkspaceProcessStep>& processSteps)
	{
		return plan.Steps.size() == processSteps.size()
		    && std::equal(
		        plan.Steps.begin(),
		        plan.Steps.end(),
		        processSteps.begin(),
		        [](const BuildWorkspaceOperationStep& planned, const BuildWorkspaceProcessStep& executable)
		        { return MatchesPlannedStep(planned, executable); });
	}

	OperationRecord RunBuildWorkspaceOperationPlan(
	    BuildWorkspaceOperationPlan plan,
	    IProcessRunner& processRunner,
	    ProcessOutputCallback outputCallback)
	{
		OperationRecord operation = plan.Operation;
		MarkOperationStarted(operation, operation.LogPath);

		if (!plan.CanRun)
		{
			operation.FailureSummary = plan.ReadinessMessages.empty() ? "Operation is not ready to run." : plan.ReadinessMessages.back();
			MarkOperationFinished(operation, OperationStatus::Failed, std::nullopt);
			return operation;
		}

		std::vector<BuildWorkspaceProcessStep> processSteps;
		try
		{
			processSteps = BuildProcessStepsForPlan(plan);
		}
		catch (const Diagnostics::Error& error)
		{
			operation.FailureSummary = std::string("Project asset-pack planning failed: ") + error.what();
			MarkOperationFinished(operation, OperationStatus::Failed, std::nullopt);
			return operation;
		}
		if (!BuildWorkspaceExecutionPlanMatches(plan, processSteps))
		{
			operation.FailureSummary = "Operation inputs changed after planning. Refresh the workflow and run it again.";
			MarkOperationFinished(operation, OperationStatus::Failed, std::nullopt);
			return operation;
		}

		for (BuildWorkspaceProcessStep& step : processSteps)
		{
			ProcessRequest request = step.Request;
			if (step.Id == "configure")
			{
				std::error_code errorCode;
				std::filesystem::create_directories(request.WorkingDirectory, errorCode);
				if (plan.Freshness.State == BuildFilesFreshnessState::GeneratorMismatch
				    || plan.Freshness.State == BuildFilesFreshnessState::FreshnessStampMismatch)
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

			ProcessResult result = processRunner.Run(request);
			if (!result.Launched || result.Canceled || result.ExitCode != 0)
			{
				if (ShouldRetryConfigureAfterDependencyRecovery(plan, step, result))
				{
					const std::string retryMessage =
					    "Detected a stale or corrupt source dependency cache. Cleaning build/_deps and retrying configure once.\n";
					if (request.OutputCallback)
					{
						request.OutputCallback(retryMessage);
					}

					std::string cleanupError;
					if (!ClearStaleConfigureState(plan, cleanupError) || !ClearSourceDependencyCache(plan, cleanupError))
					{
						operation.FailureSummary = cleanupError;
						MarkOperationFinished(operation, OperationStatus::Failed, std::nullopt);
						return operation;
					}

					result = processRunner.Run(request);
					if (result.Launched && !result.Canceled && result.ExitCode == 0)
					{
						if (request.OutputCallback)
						{
							request.OutputCallback("Source dependency cache recovery succeeded; configure completed on retry.\n");
						}
					}
				}

				if (!result.Launched || result.Canceled || result.ExitCode != 0)
				{
					operation.FailureSummary = MakeBuildWorkspaceFailureSummary(step, result);
					MarkOperationFinished(
					    operation,
					    result.Canceled ? OperationStatus::Canceled : OperationStatus::Failed,
					    result.ExitCode);
					return operation;
				}
			}

			if (step.UpdatesBuildFilesFreshness)
			{
				if (const std::optional<std::string> dependencyValidationFailure = ValidateEnabledSourceDependenciesAfterConfigure(plan))
				{
					operation.FailureSummary = *dependencyValidationFailure
					    + (step.Request.LogPath.empty() ? std::string() : " Log: " + step.Request.LogPath.string());
					MarkOperationFinished(operation, OperationStatus::Failed, 0);
					return operation;
				}

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
