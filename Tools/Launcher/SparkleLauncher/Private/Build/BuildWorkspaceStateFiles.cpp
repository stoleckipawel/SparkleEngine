#include "BuildWorkspaceStateFiles.h"

#include "Core/Public/Json/JsonReader.h"
#include "Core/Public/Strings/StringUtils.h"
#include "SparkleLauncher/LauncherPaths.h"

#include <chrono>
#include <cctype>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <sstream>

namespace SparkleLauncher
{
	std::optional<std::string> ReadCMakeCacheValue(const std::filesystem::path& cachePath, std::string_view key)
	{
		std::ifstream stream(cachePath);
		if (!stream.is_open())
		{
			return std::nullopt;
		}

		const std::string prefix = std::string(key) + ":";
		std::string line;
		while (std::getline(stream, line))
		{
			if (line.rfind(prefix, 0) == 0)
			{
				const std::size_t separator = line.find('=', prefix.size());
				if (separator != std::string::npos)
				{
					return line.substr(separator + 1);
				}
			}
		}

		return std::nullopt;
	}

	std::optional<std::string> ReadBuildFilesFreshnessStampValue(const std::filesystem::path& stampPath, std::string_view key)
	{
		std::ifstream stream(stampPath);
		if (!stream.is_open())
		{
			return std::nullopt;
		}

		std::stringstream buffer;
		buffer << stream.rdbuf();
		std::string value;
		return Json::TryReadStringProperty(buffer.str(), key, value) ? std::optional<std::string>(value) : std::nullopt;
	}

	std::optional<std::string> ReadRootCMakeProjectName(const std::filesystem::path& repositoryRoot)
	{
		std::ifstream stream(repositoryRoot / "CMakeLists.txt");
		if (!stream.is_open())
		{
			return std::nullopt;
		}

		std::string line;
		while (std::getline(stream, line))
		{
			const std::string lowerLine = Strings::ToLowerCopy(line);
			const std::size_t projectPosition = lowerLine.find("project(");
			if (projectPosition == std::string::npos)
			{
				continue;
			}

			const std::size_t nameStart = projectPosition + 8;
			std::size_t nameEnd = nameStart;
			while (nameEnd < line.size() && !std::isspace(static_cast<unsigned char>(line[nameEnd])) && line[nameEnd] != ')')
			{
				++nameEnd;
			}

			if (nameEnd > nameStart)
			{
				return line.substr(nameStart, nameEnd - nameStart);
			}
		}

		return std::nullopt;
	}

	std::filesystem::path GetBuildSolutionPath(const std::filesystem::path& repositoryRoot)
	{
		const std::filesystem::path solutionBasePath =
		    GetBuildDirectory(repositoryRoot) / ReadRootCMakeProjectName(repositoryRoot).value_or("Sparkle");
		std::filesystem::path candidate = solutionBasePath;
		candidate += ".sln";
		if (std::filesystem::exists(candidate))
		{
			return candidate;
		}

		candidate = solutionBasePath;
		candidate += ".slnx";
		if (std::filesystem::exists(candidate))
		{
			return candidate;
		}

		candidate = solutionBasePath;
		candidate += ".sln";
		return candidate;
	}

	std::string BuildUtcTimestamp()
	{
		const auto now = std::chrono::system_clock::now();
		const std::time_t time = std::chrono::system_clock::to_time_t(now);
		std::tm utcTime = {};
#if defined(_WIN32)
		gmtime_s(&utcTime, &time);
#else
		gmtime_r(&time, &utcTime);
#endif

		std::ostringstream stream;
		stream << std::put_time(&utcTime, "%Y-%m-%dT%H:%M:%SZ");
		return stream.str();
	}
}
