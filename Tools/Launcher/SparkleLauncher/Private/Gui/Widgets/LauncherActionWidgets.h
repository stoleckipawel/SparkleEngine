#pragma once

#include <QtCore/QString>
#include <QtCore/QVector>

#include <functional>

class QToolButton;
class QWidget;

namespace SparkleLauncher
{
	struct LauncherActionMenuEntry
	{
		QString Label;
		std::function<void()> Triggered;
	};

	QToolButton* CreateLauncherOverflowActionButton(
	    QWidget* parent,
	    const QString& accessibleName,
	    const QString& toolTip,
	    const QVector<LauncherActionMenuEntry>& entries);

	QString PrimaryActionLabelForOperationId(const QString& operationId);
}
