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

	QFrame* CreateHomeCapabilityCard(
	    const std::filesystem::path& repositoryRoot,
	    const QString& title,
	    const QString& status,
	    const QString& detail,
	    const QString& state,
	    QWidget* action,
	    const QString& tileRole,
	    const QString& artworkFileName,
	    QWidget* parent);

	QFrame* CreateHomeCapabilityCard(
	    const QString& title,
	    const QString& status,
	    const QString& detail,
	    const QString& state,
	    QWidget* action,
	    const QString& tileRole,
	    const QPixmap& artwork,
	    QWidget* parent);
}
