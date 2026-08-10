#pragma once

#include <QtCore/QString>
#include <QtCore/QVector>

#include <filesystem>

namespace SparkleLauncher
{
	struct LauncherCleanTarget
	{
		QString DisplayName;
		QString Path;
		QString Detail;
	};

	struct LauncherOperationRequest
	{
		std::filesystem::path RepositoryRoot;
		QString RunId;
		QString OperationId;
		QString ContentId;
		QString RunMode = "editor";
		QString EditorProfile;
		QString RuntimeProfile;
		QString WorkspaceIde;
		QString WorkspaceCompiler;
		QString SelectedTargets;
		QString RequestedLevelIds;
		QString SourceDependencyId;
		QString HostToolId;
		QString ShaderPackages;
		QString ShaderTargets;
		QString ShaderBackend;
		QString ShaderCacheDirectory;
		QString GraphicsApi;
		QString CleanScope = "cooked";
		QVector<LauncherCleanTarget> CleanTargets;
		QVector<QString> PreservedPaths;
		bool ShaderUseCache = true;
		bool ShaderEnableDebugInfo = false;
		bool ShaderEnableOptimizations = true;
		bool ShaderWarningsAsErrors = true;
		bool ShaderStripDebugInfo = true;
		bool ForceConfigure = false;
		bool ForceRecook = false;
		bool ConfirmForceRecook = false;
		bool ConfirmClean = false;
	};
}
