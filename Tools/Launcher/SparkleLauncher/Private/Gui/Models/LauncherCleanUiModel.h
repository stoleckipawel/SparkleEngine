#pragma once

#include "LauncherOperationRequest.h"

#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QVector>

#include <filesystem>

namespace SparkleLauncher
{
	struct CleanScopeUiOption
	{
		QString Label;
		QString Value;
		QString Detail;
		QString Preview;
		QString Group;
	};

	struct ActionCleanTargetContext
	{
		std::filesystem::path RepositoryRoot;
		std::filesystem::path RunningLauncherPath;
		QString OperationId;
		QString ContentId;
		QString EditorProfile;
		QString RuntimeProfile;
		QString SelectedTargets;
	};

	std::filesystem::path ResolveCleanScopePreviewPath(
	    const std::filesystem::path& repositoryRoot,
	    const QString& projectId,
	    const QString& scope);

	QString CleanScopeDisplayName(const QString& scopeValue);

	bool SupportsActionSpecificClean(const QString& operationId);

	QVector<LauncherCleanTarget> BuildActionSpecificCleanTargets(const ActionCleanTargetContext& context);

	void AddExplicitCleanTarget(
	    QVector<LauncherCleanTarget>& targets,
	    const QString& displayName,
	    const std::filesystem::path& path,
	    const QString& detail);

	void AddTargetArtifactOutputs(
	    QVector<LauncherCleanTarget>& targets,
	    const std::filesystem::path& repositoryRoot,
	    const QString& profileName,
	    const QString& targetName,
	    const QString& detail,
	    const std::filesystem::path& preservedPath = {});

	void AddProjectTargetArtifactOutputs(
	    QVector<LauncherCleanTarget>& targets,
	    const std::filesystem::path& repositoryRoot,
	    const QString& profileName,
	    const QString& projectName,
	    const QString& productRole,
	    const QString& targetName,
	    const QString& detail);
}
