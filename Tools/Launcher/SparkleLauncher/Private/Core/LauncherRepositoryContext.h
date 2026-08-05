#pragma once

#include "SparkleLauncher/RepositoryLocator.h"

#include <filesystem>
#include <optional>
#include <string>

namespace SparkleLauncher
{
	std::optional<RepositoryRoot> TryReadLauncherRepositoryContext(
	    const std::filesystem::path& launcherDirectory,
	    std::string& outErrorMessage);
}
