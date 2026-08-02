#pragma once

#include <QtCore/QString>
#include <QtCore/QVector>

namespace SparkleLauncher
{
	struct LauncherContentSummary;

	struct LauncherLevelUiEntry final
	{
		QString Id;
		QString DisplayName;
		QString Description;
		QString ThumbnailPath;
		QString Detail;
		QString Status;
		QString State;
		QString SourcePageUrl;
		QString UnsupportedReason;
		bool Selected = false;
		bool SourceReady = false;
		bool RuntimeSupported = true;
		bool Ready = false;
		bool CanSelect = true;
	};

	struct LauncherStartupLevelUiEntry final
	{
		QString Id;
		QString DisplayName;
		QString Status;
		bool Ready = false;
	};

	struct LauncherLevelUiModel final
	{
		bool Loaded = false;
		QString LoadError;
		QVector<LauncherLevelUiEntry> Levels;
		QVector<LauncherStartupLevelUiEntry> StartupLevels;

		static LauncherLevelUiModel Build(const LauncherContentSummary& content);
	};
}
