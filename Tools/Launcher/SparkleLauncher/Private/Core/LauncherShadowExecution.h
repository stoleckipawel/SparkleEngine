#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace SparkleLauncher
{
	enum class LauncherShadowStartState
	{
		AlreadyRunningFromShadow,
		Started,
		Failed,
	};

	enum class LauncherShadowCompletionPolicy
	{
		WaitForCompletion,
		ReleaseCallingArtifact,
	};

	struct LauncherShadowStartResult final
	{
		LauncherShadowStartState State = LauncherShadowStartState::Failed;
		int ExitCode = 1;
		std::string ErrorMessage;
	};

	LauncherShadowStartResult StartLauncherShadow(const std::filesystem::path& repositoryRoot, const std::vector<std::string>& arguments, LauncherShadowCompletionPolicy completionPolicy);
}
