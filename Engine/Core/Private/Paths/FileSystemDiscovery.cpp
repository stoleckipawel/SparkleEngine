#include "PCH.h"

#include "Paths/FileSystemDiscovery.h"

#include "Core/Public/FileSystemUtils.h"
#include "Core/Public/Paths/PathUtils.h"
#include "Core/Public/Strings/StringUtils.h"

#include <cctype>
#include <string_view>
#include <system_error>

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
