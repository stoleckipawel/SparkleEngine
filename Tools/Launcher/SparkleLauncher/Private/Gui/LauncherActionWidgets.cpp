#include "LauncherActionWidgets.h"

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
	namespace
	{
		QIcon CreateOverflowMenuButtonIcon(const QColor& color)
		{
			constexpr int iconExtent = 12;
			constexpr qreal dotRadius = 1.15;
			QPixmap pixmap(iconExtent, iconExtent);
			pixmap.fill(Qt::transparent);

			QPainter painter(&pixmap);
			painter.setRenderHint(QPainter::Antialiasing, true);
			painter.setPen(Qt::NoPen);
			painter.setBrush(color);
			const qreal centerX = iconExtent * 0.5;
			for (const qreal centerY : {2.5, 6.0, 9.5})
			{
				painter.drawEllipse(QPointF(centerX, centerY), dotRadius, dotRadius);
			}

			return QIcon(pixmap);
		}
	}

	QToolButton* CreateLauncherOverflowActionButton(
	    QWidget* parent,
	    const QString& accessibleName,
	    const QString& toolTip,
	    const QVector<LauncherActionMenuEntry>& entries)
	{
		QToolButton* button = new QToolButton(parent);
		button->setObjectName("DependencyActionButton");
		button->setIcon(CreateOverflowMenuButtonIcon(QColor("#c7c7c7")));
		button->setIconSize(QSize(8, 8));
		button->setToolTip(toolTip);
		button->setAccessibleName(accessibleName);
		button->setPopupMode(QToolButton::InstantPopup);
		button->setAutoRaise(true);
		button->setFixedSize(16, 16);

		QMenu* menu = new QMenu(parent);
		menu->setObjectName("OverflowMenu");
		QFont menuFont = menu->font();
		menuFont.setPointSizeF(8.0);
		menu->setFont(menuFont);
		menu->setStyleSheet("QMenu { padding: 2px 0; } QMenu::item { padding: 3px 10px 3px 8px; }");

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
		if (operationId == "toolchain.check")
		{
			return "Check";
		}
		if (operationId == "workspace.setup")
		{
			return "Sync";
		}
		if (operationId == "workspace.generate-solution")
		{
			return "Generate";
		}
		if (operationId == "package.release")
		{
			return "Assemble";
		}
		if (operationId == "workspace.clean")
		{
			return "Clean";
		}
		if (operationId == "quality.format")
		{
			return "Format";
		}
		if (operationId.startsWith("cook."))
		{
			return "Cook";
		}
		if (operationId == "workspace.build-all" || operationId == "launcher.build.self" || operationId.startsWith("project.build"))
		{
			return "Build";
		}
		if (operationId.startsWith("project.run") || operationId.startsWith("workspace.run") || operationId.startsWith("project.open"))
		{
			return "Run";
		}
		return "Run";
	}
}
