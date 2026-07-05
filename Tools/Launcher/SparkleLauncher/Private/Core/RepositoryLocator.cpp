#include "SparkleLauncher/RepositoryLocator.h"

#include "Core/Public/FileSystemUtils.h"
#include "Core/Public/Paths/PathUtils.h"

#include <system_error>

namespace SparkleLauncher
{
	bool IsRepositoryRoot(const std::filesystem::path& path)
	{
		if (path.empty())
		{
			return false;
		}

		std::error_code errorCode;
		const std::filesystem::path enginePath = path / "Engine";
		const bool hasWorkspaceMarker = std::filesystem::exists(path / std::string(Filesystem::kWorkspaceMarker), errorCode);
		errorCode.clear();
		const bool hasEngineMarker = std::filesystem::exists(enginePath / std::string(Filesystem::kEngineMarker), errorCode);
		errorCode.clear();
		const bool hasEngineDirectory = std::filesystem::is_directory(enginePath, errorCode);
		errorCode.clear();
		const bool hasToolsDirectory = std::filesystem::is_directory(path / "Tools", errorCode);
		errorCode.clear();
		const bool hasProjectsDirectory = std::filesystem::is_directory(path / "Projects", errorCode);
		return hasWorkspaceMarker && hasEngineMarker && hasEngineDirectory && hasToolsDirectory && hasProjectsDirectory;
	}

	std::optional<RepositoryRoot> TryFindRepositoryRoot(const std::filesystem::path& startPath, std::string& outErrorMessage)
	{
		outErrorMessage.clear();

		std::error_code errorCode;
		std::filesystem::path current = Paths::Normalize(startPath.empty() ? std::filesystem::current_path(errorCode) : startPath);
		if (errorCode)
		{
			outErrorMessage = "Failed to resolve current directory: " + errorCode.message();
			return std::nullopt;
		}

		if (std::filesystem::is_regular_file(current, errorCode))
		{
			current = current.parent_path();
		}

		const std::optional<std::filesystem::path> workspaceRoot =
		    Filesystem::FindAncestorWithMarker(current, Filesystem::kWorkspaceMarker);
		if (workspaceRoot && IsRepositoryRoot(*workspaceRoot))
		{
			RepositoryRoot root;
			root.RootPath = *workspaceRoot;
			root.EnginePath = root.RootPath / "Engine";
			root.ToolsPath = root.RootPath / "Tools";
			root.ProjectsPath = root.RootPath / "Projects";
			return root;
		}

		outErrorMessage = "Could not find SparkleEngine repository root marker from: " + startPath.string();
		return std::nullopt;
	}
}
