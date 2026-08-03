#include "LauncherActionWidgets.h"

#include "LauncherUiDesign.h"
#include "LauncherUiModel.h"

#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QStyle>

namespace SparkleLauncher
{
	QString SyncItemStateText(SyncItemState state)
	{
		switch (state)
		{
			case SyncItemState::Missing:
				return "Missing";
			case SyncItemState::Syncing:
				return "Syncing";
			case SyncItemState::Synced:
				return "Synced";
		}
		return "Missing";
	}

	QString SyncItemStateStyle(SyncItemState state)
	{
		switch (state)
		{
			case SyncItemState::Missing:
				return "warning";
			case SyncItemState::Syncing:
				return "running";
			case SyncItemState::Synced:
				return "ok";
		}
		return "warning";
	}

	void ApplyInlineStatusLabel(QLabel& label, const QString& text, const QString& state)
	{
		label.setObjectName("InlineStatusValue");
		label.setFixedWidth(LauncherUi::Row::InlineStatusWidth);
		label.setText(QStringLiteral("● %1").arg(text));
		label.setProperty("State", state);
		label.style()->unpolish(&label);
		label.style()->polish(&label);
	}

	void ApplyStatusActionButtonPresentation(QPushButton& button, const QString& label, const QString& state)
	{
		button.setObjectName("StatusActionButton");
		button.setFixedSize(LauncherUi::Row::StatusActionWidth, LauncherUi::Row::StatusActionHeight);
		button.setText(label);
		button.setProperty("ActionState", state);
		button.style()->unpolish(&button);
		button.style()->polish(&button);
	}

	void ApplySyncStateLabel(QLabel& label, SyncItemState state)
	{
		ApplyInlineStatusLabel(label, SyncItemStateText(state), SyncItemStateStyle(state));
	}

	void ApplySyncActionButtonState(QPushButton& button, SyncItemState state, const QString& displayName)
	{
		switch (state)
		{
			case SyncItemState::Missing:
				ApplyStatusActionButtonPresentation(button, "Sync", SyncItemStateStyle(state));
				button.setProperty("ActionIntent", "sync");
				button.setEnabled(true);
				button.setAccessibleName("Sync " + displayName);
				button.setToolTip("Sync " + displayName + ".");
				break;
			case SyncItemState::Syncing:
				ApplyStatusActionButtonPresentation(button, "Syncing...", SyncItemStateStyle(state));
				button.setProperty("ActionIntent", "none");
				button.setEnabled(false);
				button.setAccessibleName("Syncing " + displayName);
				button.setToolTip("Sync is in progress.");
				break;
			case SyncItemState::Synced:
				ApplyStatusActionButtonPresentation(button, "Clean", SyncItemStateStyle(state));
				button.setProperty("ActionIntent", "clean");
				button.setEnabled(true);
				button.setAccessibleName("Clean " + displayName);
				button.setToolTip("Clean " + displayName + ".");
				break;
		}
	}

	QString PrimaryActionLabelForOperationId(const QString& operationId)
	{
		const QString primaryVerb = LauncherUiModelForOperation(operationId).PrimaryVerb;
		return primaryVerb.isEmpty() ? QString("Run") : primaryVerb;
	}
}
