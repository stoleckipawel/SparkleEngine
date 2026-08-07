#include "SparkleLauncher/BuildWorkspaceOperations.h"

#include "NativeBuildOutputReset.h"
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

	static std::string ExtractBuildFailureDetail(std::string_view text)
	{
		std::istringstream stream{std::string(text)};
		for (std::string line; std::getline(stream, line);)
		{
			const std::string trimmedLine = TrimCopy(line);
			if (trimmedLine.find(": error ") != std::string::npos || trimmedLine.find(" error C") != std::string::npos
			    || trimmedLine.find("fatal error") != std::string::npos)
			{
				return trimmedLine;
			}
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

		if (step.Id == "configure")
		{
			return ExtractCMakeFailureDetail(text, true);
		}
		if (step.Id == "build")
		{
			return ExtractBuildFailureDetail(text);
		}
		if (step.Id == "install-host-tool"
		    && text.find("Visual Studio Installer could not continue because Visual Studio or an MSBuild process is running.")
		        != std::string::npos)
		{
			return "Visual Studio or MSBuild is running. Close active IDEs and builds, then retry.";
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
			return detail.empty() ? "Generate build files failed." + logSuffix : "Generate build files failed: " + detail + logSuffix;
		}
		if (step.Id == "build")
		{
			return detail.empty() ? "Build targets failed." + logSuffix : "Build targets failed: " + detail + logSuffix;
		}
		if (step.Id == "open-ide")
		{
			return detail.empty() ? "Open IDE failed." + logSuffix : "Open IDE failed: " + detail + logSuffix;
		}
		if (step.Id == "install-host-tool")
		{
			return detail.empty() ? "Host tool installation failed." + logSuffix : "Host tool installation failed: " + detail + logSuffix;
		}
		return detail.empty() ? step.DisplayName + " failed." + logSuffix : step.DisplayName + " failed: " + detail + logSuffix;
	}

	static std::optional<std::string> ValidateEnabledSourceDependenciesAfterConfigure(const BuildWorkspaceOperationPlan& plan)
	{
		if (!plan.Request.SourceDependencyId.empty())
		{
			return std::nullopt;
		}
		if (plan.Kind != BuildWorkspaceOperationKind::SyncCode && plan.Kind != BuildWorkspaceOperationKind::GenerateBuildFiles)
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

	static std::optional<std::string> ValidateRequestedSourceDependencyAfterConfigure(const BuildWorkspaceOperationPlan& plan)
	{
		if (plan.Request.SourceDependencyId.empty())
		{
			return std::nullopt;
		}

		const SourceDependencyEntry* dependency = FindSourceDependency(plan.Request.SourceDependencyId);
		if (dependency == nullptr)
		{
			return "Unknown source dependency: " + plan.Request.SourceDependencyId + ".";
		}

		const SourceDependencyValidation validation =
		    ValidateSourceDependency(*dependency, GetBuildDirectory(plan.RepositoryRoot) / "_deps");
		if (validation.Ready)
		{
			return std::nullopt;
		}

		std::ostringstream summary;
		summary << dependency->Label << " source cache is incomplete after sync.";
		if (!validation.MissingRelativePaths.empty())
		{
			summary << " Missing: " << validation.MissingRelativePaths.front() << ".";
		}
		return summary.str();
	}

	static std::optional<std::string> ValidateRequestedHostToolAfterInstall(const BuildWorkspaceOperationPlan& plan)
	{
		if (plan.Kind != BuildWorkspaceOperationKind::InstallHostTool)
		{
			return std::nullopt;
		}

		const BuildToolchainStatus refreshedToolchain =
		    DetectBuildToolchain(plan.RepositoryRoot, plan.Request.PreferredIde, plan.Request.Compiler);
		const auto installedTool = std::find_if(
		    refreshedToolchain.Items.begin(),
		    refreshedToolchain.Items.end(),
		    [&plan](const ToolchainItemStatus& item) { return item.Id == plan.Request.HostToolId; });
		if (installedTool != refreshedToolchain.Items.end() && installedTool->State == ToolchainItemState::Found)
		{
			return std::nullopt;
		}

		const std::string displayName =
		    installedTool == refreshedToolchain.Items.end() ? plan.Request.HostToolId : installedTool->DisplayName;
		return "The installer completed, but " + displayName
		    + " is still unavailable. The required compiler or toolchain component was not installed.";
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
			const bool exists = std::filesystem::exists(path, errorCode);
			if (errorCode)
			{
				errorMessage = "Failed to inspect stale generated build state: " + path.string() + ": " + errorCode.message();
				return false;
			}
			if (!exists)
			{
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
		const bool exists = std::filesystem::exists(dependencyCachePath, errorCode);
		if (errorCode)
		{
			errorMessage = "Failed to inspect the source dependency cache: " + dependencyCachePath.string() + ": " + errorCode.message();
			return false;
		}
		if (!exists)
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
		if (!plan.Request.SourceDependencyId.empty())
		{
			return false;
		}
		if (step.Id != "configure")
		{
			return false;
		}

		if (plan.Kind != BuildWorkspaceOperationKind::SyncCode && plan.Kind != BuildWorkspaceOperationKind::GenerateBuildFiles)
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
			operation.FailureSummary = std::string("Operation planning failed: ") + error.what();
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
				if (errorCode)
				{
					operation.FailureSummary = "Failed to prepare the configure working directory: " + request.WorkingDirectory.string()
					    + ": " + errorCode.message();
					MarkOperationFinished(operation, OperationStatus::Failed, std::nullopt);
					return operation;
				}
				if (plan.Request.SourceDependencyId.empty() && RequiresNativeBuildOutputReset(plan.Freshness.State))
				{
					if (outputCallback)
					{
						outputCallback(
						    "The selected toolchain is incompatible with existing native outputs. Resetting generated build products while "
						    "preserving downloaded sources and cooked content.\n");
					}

					std::string cleanupError;
					if (!ResetNativeBuildOutputs(plan.RepositoryRoot, plan.Freshness.BuildDirectory, cleanupError))
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

			if (const std::optional<std::string> hostToolValidationFailure = ValidateRequestedHostToolAfterInstall(plan))
			{
				operation.FailureSummary =
				    *hostToolValidationFailure + (step.Request.LogPath.empty() ? std::string() : " Log: " + step.Request.LogPath.string());
				MarkOperationFinished(operation, OperationStatus::Failed, 0);
				return operation;
			}

			if (const std::optional<std::string> dependencyValidationFailure = ValidateRequestedSourceDependencyAfterConfigure(plan))
			{
				operation.FailureSummary = *dependencyValidationFailure
				    + (step.Request.LogPath.empty() ? std::string() : " Log: " + step.Request.LogPath.string());
				MarkOperationFinished(operation, OperationStatus::Failed, 0);
				return operation;
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
