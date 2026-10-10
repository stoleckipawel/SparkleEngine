#include "LauncherRepositoryContext.h"

namespace SparkleLauncher
{
	std::optional<RepositoryRoot> TryResolveLauncherRepositoryContext(const std::filesystem::path& requestedRoot, const std::filesystem::path& launcherDirectory, std::string& outErrorMessage)
	{
		if (!requestedRoot.empty())
		{
			return TryOpenRepositoryRoot(requestedRoot, outErrorMessage);
		}

		std::error_code errorCode;
		const std::filesystem::path workingDirectory = std::filesystem::current_path(errorCode);
		if (!errorCode)
		{
			std::string workingDirectoryError;
			if (const std::optional<RepositoryRoot> repository = TryFindRepositoryRoot(workingDirectory, workingDirectoryError))
			{
				return repository;
			}
		}

		if (const std::optional<RepositoryRoot> repository = TryFindRepositoryRoot(launcherDirectory, outErrorMessage))
		{
			return repository;
		}

		outErrorMessage = "Sparkle repository not found from the working directory or Launcher location. Start the Launcher from a Sparkle checkout or pass --root <repo-root>.";
		return std::nullopt;
	}
}
