#pragma once

#include "LauncherUiModel.h"

#include <QtCore/QString>
#include <QtCore/QVector>

namespace SparkleLauncher
{
	struct LauncherWorkflowDefinition
	{
		LauncherWorkflowPageKind PageKind = LauncherWorkflowPageKind::Unknown;
		QVector<QString> OperationIds;
	};

	QString LauncherHomeOperationId();
	QVector<LauncherWorkflowDefinition> CreateLauncherWorkflowCatalog();
	QString LauncherOperationDisplayNameOverride(const QString& operationId);
	QString LauncherOperationImpactText(const QString& operationId);
	QString LauncherWorkflowPrimaryVerb(const QString& operationId);
}
