#include "PCH.h"

#include "FileSystemUtils.h"

#if defined(_WIN32)
	#define WIN32_LEAN_AND_MEAN
	#define NOMINMAX
	#include <Windows.h>
#endif

namespace Engine::FileSystem
{
	std::filesystem::path NormalizePath(const std::filesystem::path& path)
	{
		if (path.empty())
		{
			return {};
		}

		auto normalized = path.is_relative() ? std::filesystem::absolute(path) : path;
		normalized.make_preferred();

		std::error_code ec;
		if (auto canonical = std::filesystem::weakly_canonical(normalized, ec); !ec)
		{
			return canonical;
		}
		return normalized;
	}

	std::filesystem::path GetExecutableDirectory()
	{
#if defined(_WIN32)
		wchar_t buffer[MAX_PATH];
		const DWORD len = GetModuleFileNameW(nullptr, buffer, MAX_PATH);
		if (len > 0 && len < MAX_PATH)
		{
			return std::filesystem::path(buffer).parent_path();
		}
#endif
		return std::filesystem::current_path();
	}

	std::optional<std::filesystem::path> FindAncestorWithMarker(
	    const std::filesystem::path& startDir,
	    std::string_view markerFileName,
	    uint32_t maxDepth)
	{
		if (startDir.empty() || markerFileName.empty())
		{
			return std::nullopt;
		}

		std::error_code ec;
		auto currentDir = std::filesystem::weakly_canonical(startDir, ec);
		if (ec)
		{
			currentDir = startDir;
		}

		for (uint32_t depth = 0; depth < maxDepth && !currentDir.empty(); ++depth)
		{
			if (std::filesystem::exists(currentDir / markerFileName, ec))
			{
				return currentDir;
			}

			auto parentDir = currentDir.parent_path();
			if (parentDir == currentDir)
			{
				break;
			}
			currentDir = std::move(parentDir);
		}

		return std::nullopt;
	}

	std::optional<std::filesystem::path> DiscoverWorkspaceRoot()
	{
		if (auto fromExe = FindAncestorWithMarker(GetExecutableDirectory(), kWorkspaceMarker))
		{
			return NormalizePath(*fromExe);
		}

		std::error_code ec;
		if (auto fromCwd = FindAncestorWithMarker(std::filesystem::current_path(ec), kWorkspaceMarker); fromCwd && !ec)
		{
			return NormalizePath(*fromCwd);
		}

		return std::nullopt;
	}

	std::optional<std::filesystem::path> DiscoverEngineRoot()
	{
		if (auto fromExe = FindAncestorWithMarker(GetExecutableDirectory(), kEngineMarker))
		{
			return NormalizePath(*fromExe);
		}

		std::error_code ec;
		if (auto fromCwd = FindAncestorWithMarker(std::filesystem::current_path(ec), kEngineMarker); fromCwd && !ec)
		{
			return NormalizePath(*fromCwd);
		}

		if (auto workspace = DiscoverWorkspaceRoot())
		{
			auto enginePath = *workspace / "engine";
			if (std::filesystem::exists(enginePath / kEngineMarker, ec))
			{
				return NormalizePath(enginePath);
			}
		}

		return std::nullopt;
	}

	std::optional<std::filesystem::path> DiscoverProjectRoot()
	{
		std::error_code ec;
		if (auto fromCwd = FindAncestorWithMarker(std::filesystem::current_path(ec), kProjectMarker); fromCwd && !ec)
		{
			return NormalizePath(*fromCwd);
		}

		return std::nullopt;
	}
}  // namespace Engine::FileSystem
