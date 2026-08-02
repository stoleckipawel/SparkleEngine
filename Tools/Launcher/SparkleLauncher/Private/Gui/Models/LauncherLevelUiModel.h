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
		QString Family;
		QString UnsupportedReason;
		bool Synced = false;
		bool Ready = false;
		bool Selectable = true;
		bool StartupDefault = false;
	};

	struct LauncherContentPackUiEntry final
	{
		QString Id;
		QString DisplayName;
		QString Detail;
		QString Status;
		QString State;
		QString ParentPackId;
		QString SourcePageUrl;
		QString DownloadBlocker;
		bool Selected = false;
		bool Acquired = false;
		bool DownloadSupported = false;
		bool RuntimeSupported = true;
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

		static LauncherLevelUiModel Build(const LauncherProjectSummary& project);
	};
}
