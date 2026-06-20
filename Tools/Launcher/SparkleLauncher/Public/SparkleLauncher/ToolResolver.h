#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace SparkleLauncher
{
	enum class KnownTool
	{
		CMake,
		MSBuild,
		Ninja,
		Rider,
		Git,
		ClangFormat
	};

	struct ToolResolveResult
	{
		KnownTool Tool = KnownTool::CMake;
		bool Found = false;
		std::filesystem::path Path;
		std::string DisplayName;
		std::string FailureReason;
	};

	std::string ToString(KnownTool tool);
	std::vector<std::filesystem::path> GetExecutableSearchPath();
	std::optional<std::filesystem::path> FindExecutableOnPath(std::string_view executableName);
	ToolResolveResult ResolveKnownTool(KnownTool tool);
	std::filesystem::path ResolveSparkleToolPath(
	    const std::filesystem::path& repositoryRoot,
	    std::string_view profileName,
	    std::string_view executableName);
}
