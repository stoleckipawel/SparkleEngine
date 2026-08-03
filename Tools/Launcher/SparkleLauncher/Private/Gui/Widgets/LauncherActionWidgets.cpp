#include "LauncherActionWidgets.h"

#include "LauncherUiDesign.h"
#include "LauncherUiModel.h"

#include <QtGui/QColor>
#include <QtGui/QFont>
#include <QtGui/QIcon>
#include <QtGui/QPainter>
#include <QtGui/QPixmap>
#include <QtGui/QAction>
#include <QtWidgets/QLabel>
#include <QtWidgets/QMenu>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QStyle>
#include <QtWidgets/QToolButton>

namespace SparkleLauncher
{

		QIcon CreateOverflowMenuButtonIcon(const QColor& color)
		{
			QPixmap pixmap(LauncherUi::Overflow::IconExtent, LauncherUi::Overflow::IconExtent);
			pixmap.fill(Qt::transparent);

			QPainter painter(&pixmap);
			painter.setRenderHint(QPainter::Antialiasing, true);
			painter.setPen(Qt::NoPen);
			painter.setBrush(color);
			for (const qreal centerY : {LauncherUi::Overflow::DotY1, LauncherUi::Overflow::DotY2, LauncherUi::Overflow::DotY3})
			{
				painter.drawEllipse(QPointF(LauncherUi::Overflow::DotCenterX, centerY), LauncherUi::Overflow::DotRadius, LauncherUi::Overflow::DotRadius);
			}

			return QIcon(pixmap);
		}


	QToolButton* CreateLauncherOverflowActionButton(
	    QWidget* parent,
	    const QString& accessibleName,
	    const QString& toolTip,
	    const QVector<LauncherActionMenuEntry>& entries)
	{
		QToolButton* button = new QToolButton(parent);
		button->setObjectName("DependencyActionButton");
		button->setIcon(CreateOverflowMenuButtonIcon(LauncherUi::Color::Hex(LauncherUi::Color::TextBody)));
		button->setIconSize(QSize(LauncherUi::Overflow::IconSize, LauncherUi::Overflow::IconSize));
		button->setToolTip(toolTip);
		button->setAccessibleName(accessibleName);
		button->setPopupMode(QToolButton::InstantPopup);
		button->setAutoRaise(true);
		button->setFixedSize(LauncherUi::Overflow::ButtonSize, LauncherUi::Overflow::ButtonSize);

		QMenu* menu = new QMenu(parent);
		menu->setObjectName("OverflowMenu");
		QFont menuFont = menu->font();
		menuFont.setPointSizeF(LauncherUi::Overflow::MenuFontPointSize);
		menu->setFont(menuFont);

		for (const LauncherActionMenuEntry& entry : entries)
		{
			QAction* action = menu->addAction(entry.Label);
			QObject::connect(action, &QAction::triggered, button, [handler = entry.Triggered]() {
				handler();
			});
		}

		button->setMenu(menu);
		return button;
	}

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

	void ApplySyncStateLabel(QLabel& label, SyncItemState state)
	{
		label.setFixedWidth(LauncherUi::Row::SyncStatusWidth);
		label.setText(SyncItemStateText(state));
		label.setProperty("State", SyncItemStateStyle(state));
		label.style()->unpolish(&label);
		label.style()->polish(&label);
	}

	void ApplySyncActionButtonState(QPushButton& button, SyncItemState state, const QString& displayName)
	{
		button.setObjectName("SyncActionButton");
		button.setFixedSize(LauncherUi::Row::SyncActionWidth, LauncherUi::Row::SyncActionHeight);
		button.setProperty("ActionState", SyncItemStateStyle(state));
		switch (state)
		{
			case SyncItemState::Missing:
				button.setText("Sync");
				button.setProperty("ActionIntent", "sync");
				button.setEnabled(true);
				button.setAccessibleName("Sync " + displayName);
				button.setToolTip("Sync " + displayName + ".");
				break;
			case SyncItemState::Syncing:
				button.setText("Syncing...");
				button.setProperty("ActionIntent", "none");
				button.setEnabled(false);
				button.setAccessibleName("Syncing " + displayName);
				button.setToolTip("Sync is in progress.");
				break;
			case SyncItemState::Synced:
				button.setText("Clean");
				button.setProperty("ActionIntent", "clean");
				button.setEnabled(true);
				button.setAccessibleName("Clean " + displayName);
				button.setToolTip("Clean " + displayName + ".");
				break;
		}
		button.style()->unpolish(&button);
		button.style()->polish(&button);
	}

	QString PrimaryActionLabelForOperationId(const QString& operationId)
	{
		const QString primaryVerb = LauncherUiModelForOperation(operationId).PrimaryVerb;
		return primaryVerb.isEmpty() ? QString("Run") : primaryVerb;
	}
}
