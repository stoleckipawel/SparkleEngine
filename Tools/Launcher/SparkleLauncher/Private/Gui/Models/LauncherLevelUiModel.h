#pragma once

#include <QtCore/QString>
#include <QtCore/QVector>

namespace SparkleLauncher
{
	struct LauncherProjectSummary;

	struct LauncherLevelUiEntry final
	{
		QString Id;
		QString DisplayName;
		QString Detail;
		QString Status;
		QString State;
		bool Synced = false;
		bool Ready = false;
		bool Required = false;
		bool StartupDefault = false;
	};

	struct LauncherContentPackUiEntry final
	{
		QString Id;
		QString DisplayName;
		QString Detail;
		QString Status;
		QString State;
		bool Available = false;
		bool Ready = false;
		bool External = false;
	};

	struct LauncherStartupLevelUiEntry final
	{
		QString Id;
		QString DisplayName;
		QString Status;
		bool Synced = false;
		bool Ready = false;
		bool StartupDefault = false;
	};

	struct LauncherLevelUiModel final
	{
		bool Loaded = false;
		QVector<LauncherLevelUiEntry> Levels;
		QVector<LauncherContentPackUiEntry> ContentPacks;
		QVector<LauncherStartupLevelUiEntry> StartupLevels;

		static LauncherLevelUiModel Build(
		    const LauncherProjectSummary& project);
	};
}
