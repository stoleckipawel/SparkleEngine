#pragma once

#include <QtCore/QString>

namespace SparkleLauncher
{
	struct LauncherRecoveryAction
	{
		QString OperationId;
		QString Label;
		QString Detail;
		bool NavigateOnly = false;
	};

	LauncherRecoveryAction RecoveryActionForFailure(const QString& operationId, const QString& statusText);
}
