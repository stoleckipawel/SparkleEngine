#pragma once

#include "Core/Public/CoreAPI.h"

#include <filesystem>
#include <string_view>

namespace Filesystem
{
	struct WorkspaceTargetOutputPaths final
	{
		std::filesystem::path BinaryDirectory;
		std::filesystem::path LibraryDirectory;
		std::filesystem::path SymbolDirectory;
	};

	struct WorkspaceOutputPaths final
	{
		std::filesystem::path BuildRoot;
		std::filesystem::path DependencyCacheRoot;
		std::filesystem::path ArtifactRoot;
		std::filesystem::path DevelopmentArtifactRoot;
		std::filesystem::path ProjectArtifactRoot;
		std::filesystem::path DiagnosticsRoot;
		std::filesystem::path SymbolsRoot;

		SPARKLE_CORE_API WorkspaceTargetOutputPaths RuntimeSupportTargetOutputs(
		    std::string_view targetName,
		    std::string_view profileName) const;
		SPARKLE_CORE_API WorkspaceTargetOutputPaths LauncherTargetOutputs(std::string_view profileName) const;
		SPARKLE_CORE_API WorkspaceTargetOutputPaths ToolTargetOutputs(std::string_view toolName, std::string_view profileName) const;
		SPARKLE_CORE_API std::filesystem::path ToolScratchDirectory(std::string_view toolName, std::string_view profileName) const;
		SPARKLE_CORE_API std::filesystem::path LauncherBuildDirectory() const;
		SPARKLE_CORE_API std::filesystem::path SourceDependencySyncDirectory(std::string_view dependencyId) const;
		SPARKLE_CORE_API WorkspaceTargetOutputPaths
		ProjectTargetOutputs(std::string_view projectName, std::string_view productRole, std::string_view profileName) const;
		SPARKLE_CORE_API std::filesystem::path CookedProjectDirectory(std::string_view projectName) const;
		SPARKLE_CORE_API std::filesystem::path SharedCookedProjectDirectory() const;
	};

	SPARKLE_CORE_API WorkspaceOutputPaths ResolveWorkspaceOutputPaths(const std::filesystem::path& repositoryRoot);
}
