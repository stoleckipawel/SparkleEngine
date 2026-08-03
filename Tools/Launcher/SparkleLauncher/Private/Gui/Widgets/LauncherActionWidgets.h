#pragma once

#include <QtCore/QString>
#include <QtCore/QVector>

#include <functional>

class QLabel;
class QPushButton;
class QToolButton;
class QWidget;

namespace SparkleLauncher
{
	struct LauncherActionMenuEntry
	{
		QString Label;
		std::function<void()> Triggered;
	};

	enum class SyncItemState
	{
		Missing,
		Syncing,
		Synced,
	};

	QToolButton* CreateLauncherOverflowActionButton(
	    QWidget* parent,
	    const QString& accessibleName,
	    const QString& toolTip,
	    const QVector<LauncherActionMenuEntry>& entries);
	QString SyncItemStateText(SyncItemState state);
	QString SyncItemStateStyle(SyncItemState state);
	void ApplyInlineStatusLabel(QLabel& label, const QString& text, const QString& state);
	void ApplySyncStateLabel(QLabel& label, SyncItemState state);
	void ApplySyncActionButtonState(QPushButton& button, SyncItemState state, const QString& displayName);

	QString PrimaryActionLabelForOperationId(const QString& operationId);
}
