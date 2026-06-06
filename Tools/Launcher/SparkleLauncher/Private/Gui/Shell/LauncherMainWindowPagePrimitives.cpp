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
#include "LauncherProjectModel.h"
#include "LauncherRecoveryUiModel.h"
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
#include <QtWidgets/QToolButton>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

#include <cstdint>
#include <filesystem>
#include <system_error>
#include <vector>

namespace SparkleLauncher
{
	static constexpr int kSpaceSmall = LauncherUi::Space::Small;
	static constexpr int kSpaceMedium = LauncherUi::Space::Medium;
	static constexpr int kFieldLabelWidth = LauncherUi::Row::FieldLabelWidth;
	static constexpr int kStatusChipColumnWidth = LauncherUi::Row::StatusChipColumnWidth;
	static constexpr int kStatusActionColumnWidth = LauncherUi::Row::StatusActionColumnWidth;
	static constexpr const char* kColorStateReady = LauncherUi::Color::StateSuccess;
	static constexpr const char* kColorStateWarning = LauncherUi::Color::StateWarning;	void LauncherMainWindow::AddWorkflowPageHeader(QVBoxLayout& layout, const QString& operationId)
	{
		const LauncherActionHistoryRecord* history = m_actionHistory.Find(operationId);
		if (history != nullptr && history->ExitCode != 0)
		{
			const QString recoveryHint = FailureRecoveryHint(operationId, history->ResultText);
			const LauncherRecoveryAction recoveryAction = RecoveryActionForFailure(operationId, history->ResultText);
			QVBoxLayout* recoveryLayout = AddDetailsGroup(
			    layout,
			    "Current Workflow Recovery",
			    "Only shown when the selected workflow has a failed run. Raw logs stay in Activity.",
			    true);
			AddStatusRow(
			    *recoveryLayout,
			    "Current workflow recovery",
			    "Needs attention",
			    CombineStatusDetail("Last run failed: " + history->ResultText, recoveryHint),
			    "warning",
			    recoveryAction.OperationId.isEmpty() ? nullptr : CreateActionDependencyActions(recoveryAction.OperationId, recoveryAction.Label, QString(), QString(), true));
		}
	}

	void LauncherMainWindow::AddStatusRow(QVBoxLayout& layout, const QString& label, const QString& status, const QString& detail, const QString& state, QWidget* accessory)
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

		if (!detail.isEmpty())
		{
			QLabel* detailLabel = new QLabel(detail, row);
			detailLabel->setObjectName("StatusDetail");
			detailLabel->setWordWrap(true);
			textLayout->addWidget(detailLabel);
		}

		rowLayout->addLayout(textLayout, 1);

		QLabel* statusLabel = new QLabel(status, row);
		statusLabel->setObjectName("StatusValue");
		statusLabel->setProperty("State", state);
		statusLabel->setFixedWidth(kStatusChipColumnWidth);
		statusLabel->setAlignment(Qt::AlignCenter);
		rowLayout->addWidget(statusLabel, 0, Qt::AlignRight | Qt::AlignTop);

		QWidget* actionCell = new QWidget(row);
		actionCell->setObjectName("StatusActionCell");
		actionCell->setFixedWidth(kStatusActionColumnWidth);
		QHBoxLayout* actionCellLayout = new QHBoxLayout(actionCell);
		actionCellLayout->setContentsMargins(0, 0, 0, 0);
		actionCellLayout->setSpacing(0);
		actionCellLayout->setAlignment(Qt::AlignCenter | Qt::AlignTop);
		if (accessory != nullptr)
		{
			accessory->setParent(actionCell);
			actionCellLayout->addWidget(accessory, 0, Qt::AlignCenter | Qt::AlignTop);
		}
		rowLayout->addWidget(actionCell, 0, Qt::AlignRight | Qt::AlignTop);

		layout.addWidget(row);
	}

	void LauncherMainWindow::AddWorkflowVisualBanner(QVBoxLayout& layout, const QString& operationId)
	{
		const QString artworkFileName =
		    operationId == "project.run" ?
		        (m_settings.LaunchTarget() == "runtime" ? QStringLiteral("workflow-project-run-runtime.png") : VisualAssetForOperation(operationId)) :
		        VisualAssetForOperation(operationId);
		if (artworkFileName.isEmpty())
		{
			return;
		}

		QWidget* artwork = CreateLauncherVisualArtworkWidget(
		    m_repositoryRoot,
		    artworkFileName,
		    "WorkflowVisualArtwork",
		    LauncherUi::WorkflowVisual::ArtworkSize(),
		    LauncherArtworkPreset::WorkflowBanner,
		    this);
		if (artwork == nullptr)
		{
			return;
		}

		QFrame* banner = new QFrame(this);
		banner->setObjectName("WorkflowVisualBanner");
		banner->setMinimumHeight(LauncherUi::WorkflowVisual::MinHeight);
		QHBoxLayout* shellLayout = new QHBoxLayout(banner);
		shellLayout->setContentsMargins(0, 0, 0, 0);
		shellLayout->setSpacing(0);

		QWidget* copyPane = new QWidget(banner);
		copyPane->setObjectName("WorkflowVisualCopyPane");
		QVBoxLayout* copyLayout = new QVBoxLayout(copyPane);
		copyLayout->setContentsMargins(LauncherUi::WorkflowVisual::CopyMargins());
		copyLayout->setSpacing(LauncherUi::WorkflowVisual::CopySpacing);

		QLabel* title = new QLabel(VisualBannerTitleForOperation(operationId), banner);
		title->setObjectName("WorkflowVisualTitle");
		copyLayout->addWidget(title);

		QLabel* detail = new QLabel(VisualBannerTextForOperation(operationId), banner);
		detail->setObjectName("WorkflowVisualText");
		detail->setWordWrap(true);
		copyLayout->addWidget(detail, 1);

		shellLayout->addWidget(copyPane, 1);
		artwork->setParent(banner);
		shellLayout->addWidget(artwork, 0);

		layout.addWidget(banner);
	}

	QPushButton* LauncherMainWindow::CreateCommandActionButton(const QString& operationId, const QString& label, bool primary, bool runImmediately)
	{
		QPushButton* button = new QPushButton(label, this);
		button->setObjectName(primary ? "CommandPrimaryButton" : "CommandSecondaryButton");
		button->setMinimumHeight(primary ? LauncherUi::Button::PrimaryMinHeight : LauncherUi::Button::SecondaryMinHeight);
		button->setAccessibleName(label);
		button->setToolTip(runImmediately ? "Run this workflow now." : "Open this workflow.");
		RegisterFocusable(button);
		connect(button, &QPushButton::clicked, this, [this, operationId, runImmediately]() {
			SetSelectedOperation(operationId);
			if (runImmediately)
			{
				RunSelectedOperation();
			}
		});
		return button;
	}
}

