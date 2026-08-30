#include "SparkleLauncher/MaintenanceOperations.h"

#include "Core/Public/Diagnostics/Logger.h"
#include "Core/Public/Strings/StringUtils.h"
#include "MaintenanceOperationProcessRequests.h"

#include <optional>
#include <system_error>

namespace SparkleLauncher
{

	std::filesystem::path NormalizePathForCompare(const std::filesystem::path& path)
	{
		std::error_code errorCode;
		const std::filesystem::path absolutePath = std::filesystem::absolute(path, errorCode);
		return (errorCode ? path : absolutePath).lexically_normal();
	}

	bool PathsEqual(const std::filesystem::path& left, const std::filesystem::path& right)
	{
#if defined(_WIN32)
		return Strings::ToLowerCopy(left.generic_string()) == Strings::ToLowerCopy(right.generic_string());
#else
		return left == right;
#endif
	}

	bool IsSameOrDescendantPath(const std::filesystem::path& candidate, const std::filesystem::path& ancestor)
	{
		const std::filesystem::path normalizedCandidate = NormalizePathForCompare(candidate);
		const std::filesystem::path normalizedAncestor = NormalizePathForCompare(ancestor);
		auto candidateIt = normalizedCandidate.begin();
		auto ancestorIt = normalizedAncestor.begin();
		for (; ancestorIt != normalizedAncestor.end(); ++ancestorIt, ++candidateIt)
		{
			if (candidateIt == normalizedCandidate.end() || !PathsEqual(*candidateIt, *ancestorIt))
			{
				return false;
			}
		}
		return true;
	}

	static std::filesystem::path MakePlatformDeletePath(const std::filesystem::path& path)
	{
#if defined(_WIN32)
		std::error_code errorCode;
		const std::filesystem::path absolutePath = std::filesystem::absolute(path, errorCode);
		const std::filesystem::path candidate = errorCode ? path : absolutePath;
		const std::wstring nativePath = candidate.native();
		if (nativePath.rfind(LR"(\\?\)", 0) == 0)
		{
			return candidate;
		}
		if (nativePath.rfind(LR"(\\)", 0) == 0)
		{
			return std::filesystem::path(std::wstring(LR"(\\?\UNC\)") + nativePath.substr(2));
		}
		return std::filesystem::path(std::wstring(LR"(\\?\)") + nativePath);
#else
		return path;
#endif
	}

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
			errorCode.clear();
			const std::filesystem::path deletePath = MakePlatformDeletePath(path);
			if (std::filesystem::is_directory(deletePath, errorCode))
			{
				std::filesystem::remove_all(deletePath, errorCode);
			}
			else if (!errorCode)
			{
				std::filesystem::remove(deletePath, errorCode);
			}

			std::error_code existsError;
			if (errorCode || std::filesystem::exists(path, existsError))
			{
				outErrorMessage = "Clean blocked by locked files, long paths, or permissions: " + path.string();
				return false;
			}
		}

		outErrorMessage.clear();
		return true;
	}

	static bool RemovePathPreservingChildren(
	    const std::filesystem::path& path,
	    const std::vector<std::filesystem::path>& preservedPaths,
	    std::string& outErrorMessage)
	{
		for (const std::filesystem::path& preservedPath : preservedPaths)
		{
			if (PathsEqual(NormalizePathForCompare(path), NormalizePathForCompare(preservedPath)))
			{
				outErrorMessage.clear();
				return true;
			}
		}

		bool hasPreservedDescendant = false;
		for (const std::filesystem::path& preservedPath : preservedPaths)
		{
			if (IsSameOrDescendantPath(preservedPath, path))
			{
				hasPreservedDescendant = true;
				break;
			}
		}
		if (!hasPreservedDescendant)
		{
			return RemovePath(path, outErrorMessage);
		}

		std::error_code errorCode;
		if (!std::filesystem::exists(path, errorCode))
		{
			outErrorMessage.clear();
			return true;
		}
		if (!std::filesystem::is_directory(path, errorCode))
		{
			return RemovePath(path, outErrorMessage);
		}

		for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(path, errorCode))
		{
			if (errorCode)
			{
				outErrorMessage = "Clean blocked by permissions while enumerating: " + path.string();
				return false;
			}
			if (!RemovePathPreservingChildren(entry.path(), preservedPaths, outErrorMessage))
			{
				return false;
			}
		}

		const bool pathStillHasEntries = std::filesystem::directory_iterator(path, errorCode) != std::filesystem::directory_iterator();
		if (!errorCode && !pathStillHasEntries)
		{
			return RemovePath(path, outErrorMessage);
		}

		outErrorMessage.clear();
		return true;
	}

	static bool RemoveBuildDirectoryContentsPreservingDependencies(
	    const std::filesystem::path& buildDirectory,
	    std::string& outErrorMessage)
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
		static constexpr std::string_view kRiderSettingsSuffix = ".DotSettings.user";
		const bool isRiderSettingsFile = filename.size() >= kRiderSettingsSuffix.size()
		    && filename.compare(filename.size() - kRiderSettingsSuffix.size(), kRiderSettingsSuffix.size(), kRiderSettingsSuffix) == 0;
		return extension == ".sln" || extension == ".slnx" || extension == ".vcxproj" || extension == ".filters" || extension == ".user"
		    || filename == "CMakeCache.txt" || filename == "cmake_install.cmake" || filename == "Makefile" || isRiderSettingsFile;
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

	static void PruneEmptyDirectories(const std::filesystem::path& root)
	{
		std::error_code errorCode;
		if (!std::filesystem::is_directory(root, errorCode))
		{
			return;
		}

		std::vector<std::filesystem::path> directories;
		for (std::filesystem::recursive_directory_iterator
		         iterator(root, std::filesystem::directory_options::skip_permission_denied, errorCode),
		    end;
		    iterator != end;
		    iterator.increment(errorCode))
		{
			if (errorCode)
			{
				errorCode.clear();
				continue;
			}
			if (iterator->is_directory(errorCode))
			{
				directories.push_back(iterator->path());
			}
			errorCode.clear();
		}

		std::sort(
		    directories.begin(),
		    directories.end(),
		    [](const std::filesystem::path& left, const std::filesystem::path& right)
		    { return left.native().size() > right.native().size(); });

		for (const std::filesystem::path& directory : directories)
		{
			errorCode.clear();
			if (!std::filesystem::is_directory(directory, errorCode))
			{
				continue;
			}
			const bool empty = std::filesystem::directory_iterator(directory, errorCode) == std::filesystem::directory_iterator();
			if (!errorCode && empty)
			{
				std::filesystem::remove(directory, errorCode);
			}
		}
	}

	static bool RunCleanStep(
	    const MaintenanceOperationProcessStep& step,
	    const std::vector<std::filesystem::path>& preservedPaths,
	    std::string& outErrorMessage)
	{
		switch (step.CleanBehavior)
		{
			case MaintenanceCleanBehavior::RemovePath:
				return preservedPaths.empty() ? RemovePath(step.DestructivePath, outErrorMessage)
				                              : RemovePathPreservingChildren(step.DestructivePath, preservedPaths, outErrorMessage);
			case MaintenanceCleanBehavior::RemoveBuildDirectoryContentsPreservingDependencies:
				return RemoveBuildDirectoryContentsPreservingDependencies(step.DestructivePath, outErrorMessage);
			case MaintenanceCleanBehavior::RemoveRootGeneratedFiles:
				return RemoveRootGeneratedFiles(step.DestructivePath, outErrorMessage);
		}

		outErrorMessage = "Unknown clean behavior.";
		return false;
	}

	static std::string MakeCleanFailureSummary(
	    const MaintenanceOperationProcessStep& step,
	    const OperationRecord& operation,
	    const std::string& errorMessage)
	{
		std::string summary = errorMessage.empty() ? "Clean blocked by locked files or permissions." : errorMessage;
		summary += " Scope: " + step.DestructivePath.string();
		if (!operation.LogPath.empty())
		{
			summary += " Latest log: " + operation.LogPath.string();
		}
		return summary;
	}

	OperationRecord RunMaintenanceOperationPlan(
	    MaintenanceOperationPlan plan,
	    IProcessRunner& processRunner,
	    ProcessOutputCallback outputCallback)
	{
		(void) processRunner;
		(void) outputCallback;

		OperationRecord operation = plan.Operation;
		MarkOperationStarted(operation, operation.LogPath);

		if (!plan.CanRun)
		{
			operation.FailureSummary =
			    plan.ReadinessMessages.empty() ? "Clean operation is not ready to run." : plan.ReadinessMessages.front();
			MarkOperationFinished(operation, OperationStatus::Failed, std::nullopt);
			return operation;
		}

		std::vector<std::filesystem::path> preservedPaths = plan.Request.PreservedPaths;
		if (const std::optional<std::filesystem::path> activeLogFilePath = Logging::GetActiveLogFilePath(); activeLogFilePath.has_value())
		{
			preservedPaths.push_back(*activeLogFilePath);
		}
		for (MaintenanceOperationProcessStep& step : BuildMaintenanceProcessStepsForPlan(plan))
		{
			std::string errorMessage;
			if (!RunCleanStep(step, preservedPaths, errorMessage))
			{
				operation.FailureSummary = MakeCleanFailureSummary(step, operation, errorMessage);
				MarkOperationFinished(operation, OperationStatus::Failed, std::nullopt);
				return operation;
			}
		}

		PruneEmptyDirectories(plan.RepositoryRoot / "Projects");
		PruneEmptyDirectories(plan.RepositoryRoot);

		MarkOperationFinished(operation, OperationStatus::Succeeded, 0);
		return operation;
	}
}
