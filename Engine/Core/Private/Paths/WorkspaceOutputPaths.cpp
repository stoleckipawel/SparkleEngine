#include "PCH.h"

#include "Core/Public/Paths/WorkspaceOutputPaths.h"

#include "Core/Public/Paths/PathUtils.h"

#include <string>

namespace Filesystem
{
	WorkspaceOutputPaths ResolveWorkspaceOutputPaths(const std::filesystem::path& repositoryRoot)
	{
		const std::filesystem::path normalizedRoot = Paths::Normalize(repositoryRoot);
		const std::filesystem::path buildRoot = normalizedRoot / "build";
		const std::filesystem::path artifactRoot = normalizedRoot / "artifacts";
		const std::filesystem::path developmentArtifactRoot = artifactRoot / "dev";
		return WorkspaceOutputPaths{
		    .BuildRoot = buildRoot,
		    .DependencyCacheRoot = buildRoot / "_deps",
		    .ArtifactRoot = artifactRoot,
		    .DevelopmentArtifactRoot = developmentArtifactRoot,
		    .ProjectArtifactRoot = developmentArtifactRoot / "projects",
		    .DiagnosticsRoot = artifactRoot / "diagnostics",
		    .SymbolsRoot = artifactRoot / "symbols"};
	}

	WorkspaceTargetOutputPaths WorkspaceOutputPaths::RuntimeSupportTargetOutputs(
	    std::string_view targetName,
	    std::string_view profileName) const
	{
		const std::string target(targetName);
		const std::string profile(profileName);
		return WorkspaceTargetOutputPaths{
		    .BinaryDirectory = DevelopmentArtifactRoot / "runtime-support" / target / profile,
		    .LibraryDirectory = DevelopmentArtifactRoot / "libraries" / "runtime-support" / target / profile,
		    .SymbolDirectory = SymbolsRoot / "runtime-support" / target / profile};
	}

	WorkspaceTargetOutputPaths WorkspaceOutputPaths::LauncherTargetOutputs(std::string_view profileName) const
	{
		const std::string profile(profileName);
		return WorkspaceTargetOutputPaths{
		    .BinaryDirectory = DevelopmentArtifactRoot / "launcher" / profile,
		    .LibraryDirectory = DevelopmentArtifactRoot / "libraries" / "launcher" / profile,
		    .SymbolDirectory = SymbolsRoot / "launcher" / profile};
	}

	WorkspaceTargetOutputPaths WorkspaceOutputPaths::ToolTargetOutputs(
	    std::string_view toolName,
	    std::string_view profileName) const
	{
		const std::string tool(toolName);
		const std::string profile(profileName);
		return WorkspaceTargetOutputPaths{
		    .BinaryDirectory = DevelopmentArtifactRoot / "tools" / tool / profile,
		    .LibraryDirectory = DevelopmentArtifactRoot / "libraries" / "tools" / tool / profile,
		    .SymbolDirectory = SymbolsRoot / "tools" / tool / profile};
	}

	std::filesystem::path WorkspaceOutputPaths::ToolScratchDirectory(
	    std::string_view toolName,
	    std::string_view profileName) const
	{
		return BuildRoot / "private" / "tools" / std::string(toolName) / std::string(profileName);
	}

	std::filesystem::path WorkspaceOutputPaths::SourceDependencySyncDirectory(std::string_view dependencyId) const
	{
		return BuildRoot / "_dependency-sync" / std::string(dependencyId);
	}

	std::filesystem::path WorkspaceOutputPaths::LauncherBuildDirectory() const
	{
		return BuildRoot / "private" / "tools" / "SparkleLauncher";
	}

	WorkspaceTargetOutputPaths WorkspaceOutputPaths::ProjectTargetOutputs(
	    std::string_view projectName,
	    std::string_view productRole,
	    std::string_view profileName) const
	{
		const std::string project(projectName);
		const std::string role(productRole);
		const std::string profile(profileName);
		return WorkspaceTargetOutputPaths{
		    .BinaryDirectory = ProjectArtifactRoot / project / role / profile,
		    .LibraryDirectory = DevelopmentArtifactRoot / "libraries" / "projects" / project / role / profile,
		    .SymbolDirectory = SymbolsRoot / "projects" / project / role / profile};
	}

	std::filesystem::path WorkspaceOutputPaths::CookedProjectDirectory(std::string_view projectName) const
	{
		return ProjectArtifactRoot / std::string(projectName) / "cooked";
	}

	std::filesystem::path WorkspaceOutputPaths::SharedCookedProjectDirectory() const
	{
		return CookedProjectDirectory("Shared");
	}
}
