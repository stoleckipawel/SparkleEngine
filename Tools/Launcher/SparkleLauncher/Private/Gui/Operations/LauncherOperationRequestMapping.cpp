#include "LauncherOperationRequestMapping.h"

#include <QtCore/QRegularExpression>

namespace SparkleLauncher::LauncherOperationRequestMapping
{
	static std::vector<std::string> SplitList(const QString& text)
	{
		std::vector<std::string> values;
		for (const QString& part : text.split(QRegularExpression("[,;\\n]"), Qt::SkipEmptyParts))
		{
			const QString trimmed = part.trimmed();
			if (!trimmed.isEmpty())
			{
				values.push_back(trimmed.toStdString());
			}
		}
		return values;
	}

	static CleanScope ParseCleanScope(const QString& text)
	{
		if (text == "build-tree")
		{
			return CleanScope::BuildTree;
		}
		if (text == "artifacts")
		{
			return CleanScope::ArtifactOutputs;
		}
		if (text == "workspace-state")
		{
			return CleanScope::WorkspaceState;
		}
		if (text == "shader-cache")
		{
			return CleanScope::ShaderCache;
		}
		if (text == "deps")
		{
			return CleanScope::ThirdPartyDependencyCache;
		}
		if (text == "logs")
		{
			return CleanScope::Logs;
		}
		if (text == "clean-all")
		{
			return CleanScope::PristineGeneratedWorkspace;
		}
		return CleanScope::CookedOutputs;
	}

	BuildWorkspaceOperationRequest BuildWorkspace(const LauncherOperationRequest& request)
	{
		BuildWorkspaceOperationRequest mapped;
		WorkspaceIde workspaceIde = WorkspaceIde::VisualStudio;
		TryParseWorkspaceIde(request.WorkspaceIde.toStdString(), workspaceIde);
		WorkspaceCompiler workspaceCompiler = WorkspaceCompiler::Msvc;
		TryParseWorkspaceCompiler(request.WorkspaceCompiler.toStdString(), workspaceCompiler);
		mapped.RepositoryRoot = request.RepositoryRoot;
		mapped.ContentId = request.ContentId.toStdString();
		mapped.EditorProfile = request.EditorProfile.toStdString();
		mapped.RuntimeProfile = request.RuntimeProfile.toStdString();
		mapped.PreferredIde = workspaceIde;
		mapped.Compiler = workspaceCompiler;
		mapped.SelectedTargets = SplitList(request.SelectedTargets);
		mapped.RequestedLevelIds = SplitList(request.RequestedLevelIds);
		mapped.SourceDependencyId = request.SourceDependencyId.toStdString();
		mapped.HostToolId = request.HostToolId.toStdString();
		mapped.ForceConfigure = request.ForceConfigure;
		return mapped;
	}

	CookOperationRequest Cook(const LauncherOperationRequest& request)
	{
		CookOperationRequest mapped;
		mapped.RepositoryRoot = request.RepositoryRoot;
		mapped.ContentId = request.ContentId.toStdString();
		mapped.RuntimeProfile = request.RuntimeProfile.toStdString();
		mapped.Mode = request.ForceRecook ? CookMode::Force : CookMode::Incremental;
		mapped.ForceRecookConfirmed = request.ConfirmForceRecook;
		mapped.ShaderPackages = SplitList(request.ShaderPackages);
		mapped.ShaderTargets = SplitList(request.ShaderTargets);
		mapped.ShaderBackend = request.ShaderBackend.toStdString();
		mapped.ShaderCacheDirectory = request.ShaderCacheDirectory.toStdString();
		mapped.ShaderUseCache = request.ShaderUseCache;
		mapped.ShaderEnableDebugInfo = request.ShaderEnableDebugInfo;
		mapped.ShaderEnableOptimizations = request.ShaderEnableOptimizations;
		mapped.ShaderWarningsAsErrors = request.ShaderWarningsAsErrors;
		mapped.ShaderStripDebugInfo = request.ShaderStripDebugInfo;
		return mapped;
	}

	MaintenanceOperationRequest Maintenance(const LauncherOperationRequest& request)
	{
		MaintenanceOperationRequest mapped;
		mapped.RepositoryRoot = request.RepositoryRoot;
		mapped.ContentId = request.ContentId.toStdString();
		mapped.EditorProfile = request.EditorProfile.toStdString();
		mapped.RequestedCleanScope = ParseCleanScope(request.CleanScope);
		for (const QString& part : request.CleanScope.split(QRegularExpression("[,;\\n]"), Qt::SkipEmptyParts))
		{
			mapped.RequestedCleanScopes.push_back(ParseCleanScope(part.trimmed()));
		}
		if (mapped.RequestedCleanScopes.empty())
		{
			mapped.RequestedCleanScopes.push_back(CleanScope::CookedOutputs);
		}
		for (const LauncherCleanTarget& target : request.CleanTargets)
		{
			mapped.RequestedCleanTargets.push_back(
			    MaintenanceCleanPathSpec{
			        .DisplayName = target.DisplayName.toStdString(),
			        .Path = target.Path.toStdString(),
			        .Detail = target.Detail.toStdString()});
		}
		for (const QString& path : request.PreservedPaths)
		{
			if (!path.trimmed().isEmpty())
			{
				mapped.PreservedPaths.push_back(path.toStdString());
			}
		}
		mapped.DestructiveActionConfirmed = request.ConfirmClean;
		return mapped;
	}

	LaunchOperationRequest Launch(const LauncherOperationRequest& request)
	{
		LaunchOperationRequest mapped;
		mapped.RepositoryRoot = request.RepositoryRoot;
		mapped.ContentId = request.ContentId.toStdString();
		mapped.EditorProfile = request.EditorProfile.toStdString();
		mapped.RuntimeProfile = request.RuntimeProfile.toStdString();
		mapped.StartupLevel = request.StartupLevel.toStdString();
		mapped.GraphicsApi = request.GraphicsApi.toStdString();
		return mapped;
	}
}
