#include "PCH.h"

#include "Paths/FileSystemDiscovery.h"

#include "Core/Public/FileSystemUtils.h"
#include "Core/Public/Paths/PathUtils.h"
#include "Core/Public/Strings/StringUtils.h"

#include <cctype>
#include <string_view>
#include <system_error>
#include <utility>

#if defined(_WIN32)
	#define WIN32_LEAN_AND_MEAN
	#define NOMINMAX
	#include <Windows.h>
#endif

namespace Filesystem::Private
{
	std::string GetExecutableStem()
	{
		return Strings::ToLowerCopy(Filesystem::GetExecutablePath().stem().string());
	}

	std::string InferProjectNameFromExecutableStem(std::string executableStem)
	{
		if (executableStem.size() > std::string_view("editor").size() && executableStem.ends_with("editor"))
		{
			executableStem.resize(executableStem.size() - std::string_view("editor").size());
		}
		else if (executableStem.size() > std::string_view("runtime").size() && executableStem.ends_with("runtime"))
		{
			executableStem.resize(executableStem.size() - std::string_view("runtime").size());
		}

		if (executableStem.empty())
		{
			return {};
		}

		executableStem.front() = static_cast<char>(std::toupper(static_cast<unsigned char>(executableStem.front())));
		return executableStem;
	}

	std::optional<std::filesystem::path> DiscoverPackageRoot()
	{
		std::error_code ec;
		auto currentDir = std::filesystem::weakly_canonical(Filesystem::GetExecutableDirectory(), ec);
		if (ec)
		{
			currentDir = Filesystem::GetExecutableDirectory();
		}

		for (uint32_t depth = 0; depth < 8 && !currentDir.empty(); ++depth)
		{
			const bool hasPackageRoot = std::filesystem::exists(currentDir / Filesystem::kWorkspaceMarker, ec);
			ec.clear();
			const bool hasPackageManifest = std::filesystem::exists(currentDir / "manifests" / "sparkle-package-manifest.json", ec);
			ec.clear();
			const bool hasProjects = std::filesystem::exists(currentDir / "Projects", ec);
			ec.clear();
			if (hasPackageRoot && hasPackageManifest && hasProjects)
			{
				return Paths::Normalize(currentDir);
			}

			const auto parentDir = currentDir.parent_path();
			if (parentDir == currentDir)
			{
				break;
			}
			currentDir = parentDir;
		}

		return std::nullopt;
	}

	std::optional<std::filesystem::path> DiscoverPackageProjectRoot(const std::filesystem::path& packageRoot)
	{
		if (packageRoot.empty())
		{
			return std::nullopt;
		}

		const std::filesystem::path projectsRoot = packageRoot / "Projects";
		std::error_code ec;
		if (!std::filesystem::exists(projectsRoot, ec) || ec)
		{
			return std::nullopt;
		}

		const std::string projectName = InferProjectNameFromExecutableStem(GetExecutableStem());
		if (!projectName.empty())
		{
			const std::filesystem::path inferredProjectRoot = projectsRoot / projectName;
			if (std::filesystem::exists(inferredProjectRoot / "Cooked", ec) && !ec)
			{
				return Paths::Normalize(inferredProjectRoot);
			}
			ec.clear();
		}

		for (const auto& entry : std::filesystem::directory_iterator(projectsRoot, ec))
		{
			if (ec)
			{
				break;
			}
			if (entry.is_directory(ec) && !ec && std::filesystem::exists(entry.path() / "Cooked", ec) && !ec)
			{
				return Paths::Normalize(entry.path());
			}
			ec.clear();
		}

		return std::nullopt;
	}

	std::optional<std::filesystem::path> DiscoverWorkspaceProjectRoot()
	{
		const auto workspaceRoot = Filesystem::DiscoverWorkspaceRoot();
		if (!workspaceRoot)
		{
			return std::nullopt;
		}

		const std::filesystem::path projectsRoot = *workspaceRoot / "Projects";
		std::error_code ec;
		if (!std::filesystem::exists(projectsRoot, ec) || ec)
		{
			return std::nullopt;
		}

		const std::string executableStem = GetExecutableStem();
		if (executableStem.empty())
		{
			return std::nullopt;
		}
		for (const auto& entry : std::filesystem::directory_iterator(projectsRoot, ec))
		{
			if (ec)
			{
				break;
			}

			if (!entry.is_directory(ec) || ec)
			{
				ec.clear();
				continue;
			}

			const std::filesystem::path projectRoot = entry.path();
			if (!std::filesystem::exists(projectRoot / Filesystem::kProjectMarker, ec) || ec)
			{
				ec.clear();
				continue;
			}

			if (Strings::EqualsIgnoreCase(projectRoot.filename().string(), executableStem))
			{
				return Paths::Normalize(projectRoot);
			}
		}

		return std::nullopt;
	}
}

namespace Filesystem
{
	std::filesystem::path GetExecutablePath()
	{
#if defined(_WIN32)
		wchar_t buffer[MAX_PATH];
		const DWORD length = GetModuleFileNameW(nullptr, buffer, MAX_PATH);
		if (length > 0 && length < MAX_PATH)
		{
			return std::filesystem::path(buffer);
		}
#endif
		return {};
	}

	std::filesystem::path GetExecutableDirectory()
	{
		const std::filesystem::path executablePath = GetExecutablePath();
		return executablePath.empty() ? std::filesystem::current_path() : executablePath.parent_path();
	}

	std::optional<std::filesystem::path> FindAncestorWithMarker(
	    const std::filesystem::path& startDirectory,
	    std::string_view markerFileName,
	    std::uint32_t maxDepth)
	{
		if (startDirectory.empty() || markerFileName.empty())
		{
			return std::nullopt;
		}

		std::error_code errorCode;
		std::filesystem::path currentDirectory = std::filesystem::weakly_canonical(startDirectory, errorCode);
		if (errorCode)
		{
			currentDirectory = startDirectory;
		}

		for (std::uint32_t depth = 0; depth < maxDepth && !currentDirectory.empty(); ++depth)
		{
			if (std::filesystem::exists(currentDirectory / markerFileName, errorCode))
			{
				return currentDirectory;
			}

			std::filesystem::path parentDirectory = currentDirectory.parent_path();
			if (parentDirectory == currentDirectory)
			{
				break;
			}
			currentDirectory = std::move(parentDirectory);
		}
		return std::nullopt;
	}

	std::optional<std::filesystem::path> DiscoverWorkspaceRoot()
	{
		if (const auto executableRoot = FindAncestorWithMarker(GetExecutableDirectory(), kWorkspaceMarker))
		{
			return Paths::Normalize(*executableRoot);
		}

		std::error_code errorCode;
		if (const auto workingRoot = FindAncestorWithMarker(std::filesystem::current_path(errorCode), kWorkspaceMarker);
		    workingRoot && !errorCode)
		{
			return Paths::Normalize(*workingRoot);
		}
		return std::nullopt;
	}

	std::optional<std::filesystem::path> DiscoverEngineRoot()
	{
		if (const auto executableRoot = FindAncestorWithMarker(GetExecutableDirectory(), kEngineMarker))
		{
			return Paths::Normalize(*executableRoot);
		}

		std::error_code errorCode;
		if (const auto workingRoot = FindAncestorWithMarker(std::filesystem::current_path(errorCode), kEngineMarker);
		    workingRoot && !errorCode)
		{
			return Paths::Normalize(*workingRoot);
		}

		if (const auto workspaceRoot = DiscoverWorkspaceRoot())
		{
			const std::filesystem::path enginePath = *workspaceRoot / "engine";
			if (std::filesystem::exists(enginePath / kEngineMarker, errorCode))
			{
				return Paths::Normalize(enginePath);
			}
		}
		return std::nullopt;
	}

	std::optional<std::filesystem::path> DiscoverProjectRoot()
	{
		if (const auto executableRoot = FindAncestorWithMarker(GetExecutableDirectory(), kProjectMarker))
		{
			return Paths::Normalize(*executableRoot);
		}

		std::error_code errorCode;
		if (const auto workingRoot = FindAncestorWithMarker(std::filesystem::current_path(errorCode), kProjectMarker);
		    workingRoot && !errorCode)
		{
			return Paths::Normalize(*workingRoot);
		}

		if (const auto workspaceProjectRoot = Private::DiscoverWorkspaceProjectRoot())
		{
			return Paths::Normalize(*workspaceProjectRoot);
		}
		return std::nullopt;
	}

	std::filesystem::path ResolveWorkspaceRootPath()
	{
		if (const auto workspaceRoot = DiscoverWorkspaceRoot())
		{
			return Paths::Normalize(*workspaceRoot);
		}
		if (const auto engineRoot = DiscoverEngineRoot())
		{
			return Paths::Normalize(engineRoot->parent_path());
		}

		std::error_code errorCode;
		const std::filesystem::path workingDirectory = std::filesystem::current_path(errorCode);
		return !workingDirectory.empty() && !errorCode ?
		           Paths::Normalize(workingDirectory) :
		           Paths::Normalize(GetExecutableDirectory());
	}

	std::filesystem::path ResolveBuildOutputRootPath()
	{
		return Paths::Normalize(ResolveWorkspaceRootPath() / "build");
	}

	std::filesystem::path ResolveLogsRootPath()
	{
		return Paths::Normalize(ResolveWorkspaceRootPath() / "logs");
	}
}
