#include "SparkleLauncher/RepositoryLocator.h"

#include <system_error>

namespace SparkleLauncher
{
	bool IsRepositoryRoot(const std::filesystem::path& path)
	{
		std::error_code errorCode;
		return std::filesystem::exists(path / "CMakeLists.txt", errorCode) &&
		       std::filesystem::is_directory(path / "Engine", errorCode) &&
		       std::filesystem::is_directory(path / "Tools", errorCode) &&
		       std::filesystem::is_directory(path / "Projects", errorCode);
	}

	std::optional<RepositoryRoot> TryFindRepositoryRoot(const std::filesystem::path& startPath, std::string& outErrorMessage)
	{
		outErrorMessage.clear();

		std::error_code errorCode;
		std::filesystem::path current = NormalizePath(startPath.empty() ? std::filesystem::current_path(errorCode) : startPath);
		if (errorCode)
		{
			outErrorMessage = "Failed to resolve current directory: " + errorCode.message();
			return std::nullopt;
		}

		if (std::filesystem::is_regular_file(current, errorCode))
		{
			current = current.parent_path();
		}

		while (!current.empty())
		{
			if (IsRepositoryRoot(current))
			{
				RepositoryRoot root;
				root.RootPath = current;
				root.EnginePath = current / "Engine";
				root.ToolsPath = current / "Tools";
				root.ProjectsPath = current / "Projects";
				return root;
			}

			const std::filesystem::path parent = current.parent_path();
			if (parent == current)
			{
				break;
			}

			current = parent;
		}

		outErrorMessage = "Could not find SparkleEngine repository root from: " + startPath.string();
		return std::nullopt;
	}

	std::filesystem::path NormalizePath(const std::filesystem::path& path)
	{
		std::error_code errorCode;
		std::filesystem::path absolutePath = path.is_relative() ? std::filesystem::absolute(path, errorCode) : path;
		if (errorCode)
		{
			return path.lexically_normal();
		}

		std::filesystem::path canonicalPath = std::filesystem::weakly_canonical(absolutePath, errorCode);
		if (!errorCode)
		{
			return canonicalPath.lexically_normal();
		}

		return absolutePath.lexically_normal();
	}
}