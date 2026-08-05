#include "LauncherRepositoryContext.h"

#include "Core/Public/Strings/StringUtils.h"

#include <fstream>
#include <sstream>

namespace SparkleLauncher
{
	std::optional<RepositoryRoot> TryReadLauncherRepositoryContext(
	    const std::filesystem::path& launcherDirectory,
	    std::string& outErrorMessage)
	{
		const std::filesystem::path contextPath = launcherDirectory / "RepositoryRoot.txt";
		std::ifstream contextStream(contextPath);
		if (!contextStream.is_open())
		{
			outErrorMessage = "Launcher repository context could not be read: " + contextPath.string();
			return std::nullopt;
		}

		std::ostringstream contextBuffer;
		contextBuffer << contextStream.rdbuf();
		const std::string repositoryPath = Strings::TrimCopy(contextBuffer.str());
		if (repositoryPath.empty())
		{
			outErrorMessage = "Launcher repository context is empty: " + contextPath.string();
			return std::nullopt;
		}

		return TryOpenRepositoryRoot(repositoryPath, outErrorMessage);
	}
}
