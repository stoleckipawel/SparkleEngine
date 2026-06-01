#pragma once

#include <QtCore/QSize>
#include <QtGui/QIcon>
#include <QtWidgets/QFrame>
#include <QtWidgets/QLabel>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QWidget>

#include <functional>

class QListWidgetItem;

namespace SparkleLauncher
{
	struct LauncherActivityRowWidgets
	{
		QWidget* Root = nullptr;
		QFrame* Indicator = nullptr;
		QLabel* TitleLabel = nullptr;
		QLabel* StateLabel = nullptr;
	};

	struct LauncherOutputPanelWidgets
	{
		QWidget* Root = nullptr;
		QWidget* ActivityDetailsPanel = nullptr;
		QListWidget* ActivityList = nullptr;
		QLabel* SelectedRunSummary = nullptr;
		QTextEdit* OperationOutput = nullptr;
		QPushButton* CopyOutputButton = nullptr;
		QLabel* ProgressLabel = nullptr;
	};

	LauncherOutputPanelWidgets CreateLauncherOutputPanel(
	    QWidget* parent,
	    const QIcon& copyIcon,
	    const QSize& copyIconSize,
	    std::function<void(QWidget*)> registerFocusable,
	    std::function<void()> onCopyOutput,
	    std::function<void(QListWidgetItem*, QListWidgetItem*)> onCurrentRunChanged);

	LauncherActivityRowWidgets CreateLauncherActivityRow(QWidget* parent, const QString& title);
}
