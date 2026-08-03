#pragma once

#include <QtCore/QString>

class QLabel;
class QPushButton;

namespace SparkleLauncher
{
	enum class SyncItemState
	{
		Missing,
		Syncing,
		Synced,
	};

	QString SyncItemStateText(SyncItemState state);
	QString SyncItemStateStyle(SyncItemState state);
	void ApplyInlineStatusLabel(QLabel& label, const QString& text, const QString& state);
	void ApplyStatusActionButtonPresentation(QPushButton& button, const QString& label, const QString& state);
	void ApplySyncStateLabel(QLabel& label, SyncItemState state);
	void ApplySyncActionButtonState(QPushButton& button, SyncItemState state, const QString& displayName);

	QString PrimaryActionLabelForOperationId(const QString& operationId);
}
