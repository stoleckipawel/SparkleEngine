#include "LauncherActionWidgets.h"

#include "LauncherUiDesign.h"
#include "LauncherUiModel.h"

#include <QtGui/QColor>
#include <QtGui/QFont>
#include <QtGui/QIcon>
#include <QtGui/QPainter>
#include <QtGui/QPixmap>
#include <QtWidgets/QMenu>
#include <QtWidgets/QToolButton>
#include <QtGui/QAction>

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

	QString PrimaryActionLabelForOperationId(const QString& operationId)
	{
		const QString primaryVerb = LauncherUiModelForOperation(operationId).PrimaryVerb;
		return primaryVerb.isEmpty() ? QString("Run") : primaryVerb;
	}
}
