#pragma once

#include <QtCore/QMetaType>
#include <QtCore/QString>

namespace SparkleLauncher
{
	struct LauncherOperationResult final
	{
		QString Status;
		QString ProblemKind;
		QString Problem;
		QString ExpectedAction;
		QString LogPath;
		int ExitCode = 0;
		bool HasExitCode = false;
		bool Succeeded = false;
		bool Skipped = false;
		bool Canceled = false;
	};
}

Q_DECLARE_METATYPE(SparkleLauncher::LauncherOperationResult)
