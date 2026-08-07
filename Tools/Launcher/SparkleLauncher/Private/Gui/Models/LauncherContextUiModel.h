#pragma once

#include <QtCore/QString>
#include <QtCore/QVector>

namespace SparkleLauncher
{
	struct BuildToolchainStatus;

	struct LauncherSelectionOption final
	{
		QString DisplayName;
		QString Value;
		QString Detail;
		bool Available = false;
	};

	struct LauncherContextUiModel final
	{
		QVector<LauncherSelectionOption> GraphicsApis;
		QVector<LauncherSelectionOption> ShaderBackends;
		QVector<LauncherSelectionOption> BuildConfigurations;
		QVector<LauncherSelectionOption> Compilers;
		QVector<LauncherSelectionOption> Ides;

		static LauncherContextUiModel Build(const BuildToolchainStatus& toolchain);
	};
}
