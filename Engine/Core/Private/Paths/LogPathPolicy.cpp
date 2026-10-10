#include "PCH.h"

#include "Paths/LogPathPolicy.h"

#include "Core/Public/Paths/PathFormatting.h"
#include "Core/Public/Paths/PathUtils.h"
#include "Core/Public/Paths/ProductUserStatePaths.h"
#include "FileSystemDiscovery.h"
#include "UserStatePaths.h"

#include <filesystem>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>

namespace Paths::Private
{
	static std::filesystem::path DefaultLogDirectory(const std::filesystem::path& logsRoot, std::string_view executableStem)
	{
		const std::string sanitizedExecutableStem = PathFormatting::SanitizePathSegment(executableStem.empty() ? "Sparkle" : executableStem);
		std::filesystem::path logDirectory;
		if (PathFormatting::EndsWithIgnoreCase(sanitizedExecutableStem, "Editor") || PathFormatting::EndsWithIgnoreCase(sanitizedExecutableStem, "Runtime"))
		{
			std::string projectName = Filesystem::Private::InferProjectNameFromExecutableStem(sanitizedExecutableStem);
			if (projectName.empty())
			{
				projectName = sanitizedExecutableStem;
			}
			logDirectory = logsRoot / "Projects" / projectName / "Full";
		}
		else
		{
			logDirectory = logsRoot / "Processes" / sanitizedExecutableStem / "Full";
		}

		return logDirectory;
	}

	std::filesystem::path ResolveBootstrapLogFile(std::string_view configuredFile, LogParentDirectoryPolicy parentDirectoryPolicy)
	{
		const Filesystem::ProductUserStatePaths userState = Filesystem::Private::ResolveCurrentProductUserStatePaths();
		const std::filesystem::path& logsRoot = userState.LogsRoot;
		std::filesystem::path configuredPath{std::string(configuredFile)};
		std::filesystem::path logPath;
		if (!configuredPath.empty())
		{
			if (!configuredPath.is_absolute())
			{
				configuredPath = Paths::Normalize(logsRoot / configuredPath);
			}
			if (Paths::IsUnderRoot(configuredPath, logsRoot))
			{
				logPath = std::move(configuredPath);
			}
		}

		if (logPath.empty())
		{
			const std::string executableStem = PathFormatting::SanitizePathSegment(Filesystem::Private::GetExecutableStem());
			logPath = DefaultLogDirectory(logsRoot, executableStem) / PathFormatting::TimestampedFileName(executableStem, ".log");
		}

		if (parentDirectoryPolicy == LogParentDirectoryPolicy::EnsureExists)
		{
			std::error_code errorCode;
			std::filesystem::create_directories(logPath.parent_path(), errorCode);
		}
		return logPath;
	}
}
