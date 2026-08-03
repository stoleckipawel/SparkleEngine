#include "LauncherMainWindow.h"

#include "LauncherActionWidgets.h"
#include "LauncherArtworkWidgets.h"
#include "LauncherCleanUiModel.h"
#include "LauncherDependencyUiModel.h"
#include "LauncherHomeWidgets.h"
#include "LauncherLayoutWidgets.h"
#include "LauncherOperationRequestFactory.h"
#include "LauncherOutputWidgets.h"
#include "LauncherPageUtilities.h"
#include "LauncherContentModel.h"
#include "LauncherSettings.h"
#include "LauncherToolchainUiModel.h"
#include "LauncherUiDesign.h"
#include "LauncherUiModel.h"
#include "LauncherWorkflowCatalog.h"

#include "SparkleLauncher/BuildWorkspaceOperations.h"
#include "SparkleLauncher/CookOperations.h"
#include "SparkleLauncher/LauncherPaths.h"
#include "SparkleLauncher/LaunchOperations.h"
#include "SparkleLauncher/MaintenanceOperations.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QProcess>
#include <QtCore/QRegularExpression>
#include <QtCore/QStringList>
#include <QtGui/QGuiApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QFrame>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QScrollArea>
#include <QtWidgets/QSizePolicy>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <system_error>
#include <vector>

namespace SparkleLauncher
{
	static constexpr int kSpaceSmall = LauncherUi::Space::Small;
	static constexpr int kSpaceMedium = LauncherUi::Space::Medium;

	QLabel* LauncherMainWindow::AddStatusRow(
	    QVBoxLayout& layout,
	    const QString& label,
	    const QString& status,
	    const QString& detail,
	    const QString& state,
	    QWidget* accessory)
	{
		QFrame* row = new QFrame(this);
		row->setObjectName("StatusRow");
		QHBoxLayout* rowLayout = new QHBoxLayout(row);
		rowLayout->setContentsMargins(0, 0, 0, 0);
		rowLayout->setSpacing(kSpaceMedium);

		QVBoxLayout* textLayout = new QVBoxLayout();
		textLayout->setContentsMargins(0, 0, 0, 0);
		textLayout->setSpacing(LauncherUi::Option::StatusDetailSpacing);

		QLabel* nameLabel = new QLabel(label, row);
		nameLabel->setObjectName("StatusLabel");
		textLayout->addWidget(nameLabel);

		QHBoxLayout* metadataLayout = new QHBoxLayout();
		metadataLayout->setContentsMargins(0, 0, 0, 0);
		metadataLayout->setSpacing(kSpaceSmall);

		QLabel* statusLabel = new QLabel(row);
		ApplyInlineStatusLabel(*statusLabel, status, state);
		statusLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
		metadataLayout->addWidget(statusLabel, 0, Qt::AlignVCenter);

		if (!detail.isEmpty())
		{
			QLabel* detailLabel = new QLabel(detail, row);
			detailLabel->setObjectName("StatusDetail");
			detailLabel->setWordWrap(true);
			metadataLayout->addWidget(detailLabel, 1, Qt::AlignVCenter);
		}
		textLayout->addLayout(metadataLayout);

		rowLayout->addLayout(textLayout, 1);

		if (accessory != nullptr)
		{
			QWidget* actionCell = new QWidget(row);
			actionCell->setObjectName("StatusActionCell");
			const int actionWidth = std::max(LauncherUi::Row::StatusActionWidth, accessory->sizeHint().width());
			actionCell->setFixedWidth(actionWidth);
			QHBoxLayout* actionCellLayout = new QHBoxLayout(actionCell);
			actionCellLayout->setContentsMargins(0, 0, 0, 0);
			actionCellLayout->setSpacing(0);
			actionCellLayout->setAlignment(Qt::AlignCenter);
			accessory->setParent(actionCell);
			actionCellLayout->addWidget(accessory, 0, Qt::AlignCenter);
			rowLayout->addWidget(actionCell, 0, Qt::AlignRight | Qt::AlignVCenter);
		}

		layout.addWidget(row);
		return statusLabel;
	}

	QPushButton* LauncherMainWindow::CreateCommandActionButton(
	    const QString& operationId,
	    const QString& label,
	    bool primary,
	    bool runImmediately)
	{
		QPushButton* button = new QPushButton(label, this);
		button->setObjectName(primary ? "CommandPrimaryButton" : "CommandSecondaryButton");
		button->setMinimumHeight(primary ? LauncherUi::Button::PrimaryMinHeight : LauncherUi::Button::SecondaryMinHeight);
		button->setAccessibleName(label);
		button->setToolTip(runImmediately ? "Run this workflow now." : "Open this workflow.");
		RegisterFocusable(button);
		connect(
		    button,
		    &QPushButton::clicked,
		    this,
		    [this, operationId, runImmediately]()
		    {
			    SetSelectedOperation(operationId);
			    if (runImmediately)
			    {
				    RunSelectedOperation();
			    }
		    });
		return button;
	}
}
