#pragma once

#include "LauncherArtworkWidgets.h"

#include "SparkleLauncher/BuildWorkspaceOperations.h"

#include <QtCore/QSize>
#include <QtCore/QString>

#include <filesystem>

class QFrame;
class QPixmap;
class QWidget;

namespace SparkleLauncher
{
	std::filesystem::path FindLauncherVisualAsset(const std::filesystem::path& repositoryRoot, const QString& fileName);
	QPixmap CreateIdeQuickStartArtwork(const std::filesystem::path& repositoryRoot, WorkspaceIde ide);

	QWidget* CreateLauncherVisualArtworkWidget(
	    const std::filesystem::path& repositoryRoot,
	    const QString& fileName,
	    const QString& objectName,
	    const QSize& minimumSize,
	    LauncherArtworkPreset preset,
	    QWidget* parent);

	QFrame* CreateHomeHeroCard(
	    const std::filesystem::path& repositoryRoot,
	    const QString& status,
	    const QString& detail,
	    const QString& state,
	    QWidget* primaryAction,
	    QWidget* secondaryAction,
	    const QString& artworkFileName,
	    QWidget* parent);

	QFrame* CreateQuickStartCard(
	    const std::filesystem::path& repositoryRoot,
	    const QString& title,
	    QWidget* action,
	    const QString& artworkFileName,
	    QWidget* parent);

	QFrame* CreateQuickStartCard(const QString& title, QWidget* action, const QPixmap& artwork, QWidget* parent);
}
