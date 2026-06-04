#pragma once

#include <QtCore/QString>
#include <QtCore/QVector>

namespace SparkleLauncher
{
	struct LauncherWorkflowDefinition
	{
		QString Title;
		QString Subtitle;
		QVector<QString> OperationIds;
		QString IconKey;
	};

	QString LauncherHomeOperationId();
	QString LauncherSystemOperationId();
	QVector<LauncherWorkflowDefinition> CreateLauncherWorkflowCatalog();
	QString LauncherOperationDisplayNameOverride(const QString& operationId);
	QString LauncherOperationImpactText(const QString& operationId);
	QString LauncherWorkflowPrimaryVerb(const QString& operationId);
}
