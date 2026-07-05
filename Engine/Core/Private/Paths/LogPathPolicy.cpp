#include "PCH.h"

#include "Paths/LogPathPolicy.h"

#include "Core/Public/FileSystemUtils.h"
#include "Core/Public/Paths/PathFormatting.h"

#include <system_error>

namespace Paths::Private
{
	std::string InferProjectNameFromExecutableStem(std::string_view executableStem)
	{
		std::string projectName(executableStem.empty() ? "Sparkle" : executableStem);
		if (PathFormatting::EndsWithIgnoreCase(projectName, "Editor"))
		{
			projectName.resize(projectName.size() - std::string_view("Editor").size());
		}
		else if (PathFormatting::EndsWithIgnoreCase(projectName, "Runtime"))
		{
			projectName.resize(projectName.size() - std::string_view("Runtime").size());
		}

		return PathFormatting::SanitizePathSegment(projectName.empty() ? executableStem : projectName);
	}

	std::filesystem::path DefaultLogDirectory(bool ensureParentExists, std::string_view executableStem)
	{
		const std::string sanitizedExecutableStem =
		    PathFormatting::SanitizePathSegment(executableStem.empty() ? "Sparkle" : executableStem);
		const std::filesystem::path logsRoot = Filesystem::ResolveLogsRootPath();
		std::filesystem::path logDirectory;
		if (PathFormatting::EndsWithIgnoreCase(sanitizedExecutableStem, "Editor") ||
		    PathFormatting::EndsWithIgnoreCase(sanitizedExecutableStem, "Runtime"))
		{
			logDirectory =
			    logsRoot / "Projects" / InferProjectNameFromExecutableStem(sanitizedExecutableStem) / "Full";
		}
		else
		{
			logDirectory = logsRoot / "Processes" / sanitizedExecutableStem / "Full";
		}

		if (ensureParentExists)
		{
			std::error_code errorCode;
			std::filesystem::create_directories(logDirectory, errorCode);
		}
		return logDirectory;
	}
}
