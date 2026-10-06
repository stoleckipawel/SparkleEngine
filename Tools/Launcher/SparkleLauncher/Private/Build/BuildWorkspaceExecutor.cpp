#include "SparkleLauncher/BuildWorkspaceOperations.h"

#include "NativeBuildOutputReset.h"
#include "BuildWorkspaceProcessRequests.h"
#include "LauncherOperationProgress.h"
#include "Core/Public/Diagnostics/Error.h"
#include "Core/Public/Paths/WorkspaceOutputPaths.h"
#include "SparkleLauncher/SourceDependencyState.h"

#include <algorithm>
#include <array>
#include "Core/Public/Strings/StringUtils.h"
#include <fstream>
#include <optional>
#include <sstream>
#include <string_view>
#include <system_error>
#include <vector>

namespace SparkleLauncher
{
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
		    "SPIRV_TOOLS_",
		    "SPIRV-Tools-shared",
		    "spirv-tools/libspirv.hpp",
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
			lines.push_back(Strings::TrimCopy(line));
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
		if (step.Id == "install-host-tool")
		{
			static constexpr std::string_view installerFailurePrefix = "Visual Studio Installer exited with code ";
			const std::size_t failureStart = text.find(installerFailurePrefix);
			if (failureStart != std::string::npos)
			{
				const std::size_t failureEnd = text.find_first_of("\r\n", failureStart);
				return text.substr(failureStart, failureEnd - failureStart);
			}
		}
		return {};
	}

	static std::string MakeBuildWorkspaceFailureSummary(const BuildWorkspaceProcessStep& step, const ProcessResult& result)
	{
		if (!result.FailureReason.empty())
		{
			return result.FailureReason;
		}
		const std::string detail = ExtractFailureDetail(step, result);
		if (step.Id == "configure")
		{
			return detail.empty() ? "Generate build files failed." : "Generate build files failed: " + detail;
		}
		if (step.Id == "build")
		{
			return detail.empty() ? "Build targets failed." : "Build targets failed: " + detail;
		}
		if (step.Id == "install-host-tool")
		{
			return detail.empty() ? "Host tool installation failed." : "Host tool installation failed: " + detail;
		}
		return detail.empty() ? step.DisplayName + " failed." : step.DisplayName + " failed: " + detail;
	}

	static std::string BuildWorkspaceRecoveryAction(const BuildWorkspaceProcessStep& step)
	{
		if (step.Id == "install-host-tool")
		{
			return "Close active Visual Studio builds, correct the first installer error in the named log, then retry Install.";
		}
		if (step.Id == "configure")
		{
			return "Correct the first configure or dependency error in the named log, then retry Generate Build Files.";
		}
		if (step.Id == "build")
		{
			return "Correct the first compiler, linker, or tool error in the named log, then retry this build.";
		}
		return "Correct the first tool error in the named log, then retry this operation.";
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

		const Filesystem::WorkspaceOutputPaths workspaceOutputs = Filesystem::ResolveWorkspaceOutputPaths(plan.RepositoryRoot);
		const SourceDependencyInventoryStatus status = InspectSourceDependencyCache(workspaceOutputs.DependencyCacheRoot);
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

		const Filesystem::WorkspaceOutputPaths workspaceOutputs = Filesystem::ResolveWorkspaceOutputPaths(plan.RepositoryRoot);
		const SourceDependencyValidation validation = ValidateSourceDependency(*dependency, workspaceOutputs.DependencyCacheRoot);
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
		const Filesystem::WorkspaceOutputPaths workspaceOutputs = Filesystem::ResolveWorkspaceOutputPaths(plan.RepositoryRoot);
		const std::filesystem::path dependencyCachePath = workspaceOutputs.DependencyCacheRoot;
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

	static bool PrepareBuildWorkspaceExecution(
	    const BuildWorkspaceOperationPlan& plan,
	    OperationRecord& operation,
	    std::vector<BuildWorkspaceProcessStep>& processSteps)
	{
		if (!plan.CanRun)
		{
			SetOperationFailure(
			    operation,
			    OperationProblemKind::Prerequisite,
			    plan.ReadinessMessages.empty() ? "Operation is not ready to run." : plan.ReadinessMessages.back(),
			    "Open Sync, resolve the reported prerequisite, then retry this workflow.");
			MarkOperationFinished(operation, OperationStatus::Failed, std::nullopt);
			return false;
		}

		try
		{
			processSteps = BuildProcessStepsForPlan(plan);
		}
		catch (const Diagnostics::Error& error)
		{
			SetOperationFailure(
			    operation,
			    OperationProblemKind::Planning,
			    std::string("Operation planning failed: ") + error.what(),
			    "Refresh the workflow, review its preview, then retry.");
			MarkOperationFinished(operation, OperationStatus::Failed, std::nullopt);
			return false;
		}
		if (!BuildWorkspaceExecutionPlanMatches(plan, processSteps))
		{
			SetOperationFailure(
			    operation,
			    OperationProblemKind::Planning,
			    "Operation inputs changed after planning.",
			    "Refresh the workflow, review its updated preview, then retry.");
			MarkOperationFinished(operation, OperationStatus::Failed, std::nullopt);
			return false;
		}

		return true;
	}

	static bool RunBuildWorkspaceStep(
	    const BuildWorkspaceOperationPlan& plan,
	    const BuildWorkspaceProcessStep& step,
	    IProcessRunner& processRunner,
	    const ProcessOutputCallback& outputCallback,
	    OperationRecord& operation)
	{
		ProcessRequest request = step.Request;
		if (step.Id == "configure")
		{
			std::error_code errorCode;
			std::filesystem::create_directories(request.WorkingDirectory, errorCode);
			if (errorCode)
			{
				SetOperationFailure(
				    operation,
				    OperationProblemKind::Filesystem,
				    "Failed to prepare the configure working directory: " + request.WorkingDirectory.string() + ": " + errorCode.message(),
				    "Verify that the build directory is writable and not locked, then retry Generate Build Files.");
				MarkOperationFinished(operation, OperationStatus::Failed, std::nullopt);
				return false;
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
					SetOperationFailure(
					    operation,
					    OperationProblemKind::Filesystem,
					    std::move(cleanupError),
					    "Close processes using generated build outputs, verify path permissions, then retry.");
					MarkOperationFinished(operation, OperationStatus::Failed, std::nullopt);
					return false;
				}
			}
		}

		AppendProcessOutputCallback(request, outputCallback);

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
					SetOperationFailure(
					    operation,
					    OperationProblemKind::Filesystem,
					    std::move(cleanupError),
					    "Close processes using the dependency cache, verify path permissions, then retry Sync.");
					MarkOperationFinished(operation, OperationStatus::Failed, std::nullopt);
					return false;
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
				if (result.Canceled)
				{
					SetOperationFailure(
					    operation,
					    OperationProblemKind::Cancellation,
					    step.DisplayName + " was canceled.",
					    "Run the operation again when ready.");
				}
				else
				{
					SetProcessOperationFailure(
					    operation,
					    result.StartFailure,
					    MakeBuildWorkspaceFailureSummary(step, result),
					    BuildWorkspaceRecoveryAction(step));
				}
				MarkOperationFinished(operation, result.Canceled ? OperationStatus::Canceled : OperationStatus::Failed, result.ExitCode);
				return false;
			}
		}
		return true;
	}

	static bool ValidateBuildWorkspaceStep(
	    const BuildWorkspaceOperationPlan& plan,
	    const BuildWorkspaceProcessStep& step,
	    OperationRecord& operation)
	{
		if (const std::optional<std::string> hostToolValidationFailure = ValidateRequestedHostToolAfterInstall(plan))
		{
			SetOperationFailure(
			    operation,
			    OperationProblemKind::OutputValidation,
			    *hostToolValidationFailure,
			    "Review the installer result, ensure the requested compiler and Visual Studio toolset are selected, then retry "
			    "Install.");
			MarkOperationFinished(operation, OperationStatus::Failed, 0);
			return false;
		}

		if (const std::optional<std::string> dependencyValidationFailure = ValidateRequestedSourceDependencyAfterConfigure(plan))
		{
			SetOperationFailure(
			    operation,
			    OperationProblemKind::OutputValidation,
			    *dependencyValidationFailure,
			    "Correct the first dependency sync error in the named log, then retry this dependency Sync.");
			MarkOperationFinished(operation, OperationStatus::Failed, 0);
			return false;
		}

		if (step.UpdatesBuildFilesFreshness)
		{
			if (const std::optional<std::string> dependencyValidationFailure = ValidateEnabledSourceDependenciesAfterConfigure(plan))
			{
				SetOperationFailure(
				    operation,
				    OperationProblemKind::OutputValidation,
				    *dependencyValidationFailure,
				    "Correct the first dependency configure error in the named log, then retry Sync Code.");
				MarkOperationFinished(operation, OperationStatus::Failed, 0);
				return false;
			}

			std::string errorMessage;
			if (!UpdateBuildFilesFreshnessStamp(plan.RepositoryRoot, plan.Toolchain, errorMessage))
			{
				SetOperationFailure(
				    operation,
				    OperationProblemKind::Filesystem,
				    std::move(errorMessage),
				    "Verify write permission for the launcher state directory, then retry Generate Build Files.");
				MarkOperationFinished(operation, OperationStatus::Failed, std::nullopt);
				return false;
			}
		}
		return true;
	}

	OperationRecord RunBuildWorkspaceOperationPlan(
	    BuildWorkspaceOperationPlan plan,
	    IProcessRunner& processRunner,
	    const ProcessOutputCallback& outputCallback)
	{
		OperationRecord operation = plan.Operation;
		MarkOperationStarted(operation, operation.LogPath);
		std::vector<BuildWorkspaceProcessStep> processSteps;
		if (!PrepareBuildWorkspaceExecution(plan, operation, processSteps))
		{
			return operation;
		}

		for (std::size_t stepIndex = 0; stepIndex < processSteps.size(); ++stepIndex)
		{
			const BuildWorkspaceProcessStep& step = processSteps[stepIndex];
			ReportOperationProgress(outputCallback, step.DisplayName, stepIndex, processSteps.size());
			if (!RunBuildWorkspaceStep(plan, step, processRunner, outputCallback, operation)
			    || !ValidateBuildWorkspaceStep(plan, step, operation))
			{
				return operation;
			}
			ReportOperationProgress(outputCallback, step.DisplayName, stepIndex + 1, processSteps.size());
		}
		MarkOperationFinished(operation, OperationStatus::Succeeded, 0);
		return operation;
	}
}
