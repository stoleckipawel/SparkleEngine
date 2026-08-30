#include "LauncherMainWindow.h"

#include "LauncherActivityPanel.h"
#include "LauncherWorkflowPanel.h"
#include "LauncherActionWidgets.h"
#include "LauncherArtworkWidgets.h"
#include "LauncherBackend.h"
#include "LauncherCleanUiModel.h"
#include "LauncherDependencyUiModel.h"
#include "LauncherHomeWidgets.h"
#include "LauncherIconLibrary.h"
#include "LauncherLayoutWidgets.h"
#include "LauncherLevelUiModel.h"
#include "LauncherOperationRequestFactory.h"
#include "LauncherOperationRequestMapping.h"
#include "LauncherContentModel.h"
#include "LauncherSettings.h"
#include "LauncherToolchainUiModel.h"
#include "LauncherUiDesign.h"
#include "LauncherUiModel.h"
#include "LauncherVisualStyle.h"

#include "SparkleLauncher/BuildWorkspaceOperations.h"
#include "SparkleLauncher/CookOperations.h"
#include "SparkleLauncher/LauncherPaths.h"
#include "SparkleLauncher/MaintenanceOperations.h"

#include "Core/Public/Diagnostics/Error.h"

#include <QtCore/QSignalBlocker>
#include <QtCore/QCoreApplication>
#include <QtCore/QPointer>
#include <QtCore/QProcess>
#include <QtCore/QRegularExpression>
#include <QtCore/QStringList>
#include <QtCore/QTimer>
#include <QtCore/Qt>
#include <QtCore/QDateTime>
#include <QtGui/QBrush>
#include <QtGui/QClipboard>
#include <QtGui/QColor>
#include <QtGui/QDesktopServices>
#include <QtGui/QFont>
#include <QtGui/QGuiApplication>
#include <QtGui/QImage>
#include <QtGui/QKeyEvent>
#include <QtGui/QKeySequence>
#include <QtGui/QMouseEvent>
#include <QtGui/QPixmap>
#include <QtGui/QResizeEvent>
#include <QtGui/QTextCursor>
#include <QtGui/QTextDocument>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QScrollArea>
#include <QtWidgets/QScrollBar>
#include <QtWidgets/QSizePolicy>
#include <QtWidgets/QStyle>
#include <QtWidgets/QWidget>
#include <QtCore/QUrl>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <system_error>
#include <utility>

namespace SparkleLauncher
{
	static constexpr int kSpaceTiny = LauncherUi::Space::Tiny;
	static constexpr int kSpaceSmall = LauncherUi::Space::Small;
	static constexpr int kSpaceMedium = LauncherUi::Space::Medium;
	static constexpr int kSpaceLarge = LauncherUi::Space::Large;
	static constexpr int kPanelHorizontalMargin = LauncherUi::Shell::PanelHorizontalMargin;
	static constexpr int kPanelVerticalMargin = LauncherUi::Shell::PanelVerticalMargin;
	static constexpr int kFieldLabelWidth = LauncherUi::Row::FieldLabelWidth;
	static constexpr int kLauncherIconSize = LauncherUi::Icon::DefaultSize;
	static constexpr int kLauncherMinimumWidth = LauncherUi::Window::MinimumWidth;
	static constexpr int kLauncherMinimumHeight = LauncherUi::Window::MinimumHeight;
	static constexpr int kLauncherInitialWidth = LauncherUi::Window::InitialWidth;
	static constexpr int kLauncherInitialHeight = LauncherUi::Window::InitialHeight;
	static constexpr const char* kColorStateRunning = LauncherUi::Color::StateRunning;
	static constexpr const char* kColorStateSuccess = LauncherUi::Color::StateSuccess;
	static constexpr const char* kColorStateDestructive = LauncherUi::Color::StateDestructive;
	static constexpr const char* kColorStateWarning = LauncherUi::Color::StateWarning;
	static constexpr qint64 kActivationRefreshIntervalMs = 1500;
	static QString FirstReadinessContaining(const std::vector<std::string>& messages, const QString& needle)
	{
		for (const std::string& message : messages)
		{
			const QString text = QString::fromStdString(message);
			if (text.contains(needle, Qt::CaseInsensitive))
			{
				return text;
			}
		}
		return QString();
	}

	static QString FirstBlockingReadinessMessage(const BuildWorkspaceOperationPlan& plan)
	{
		for (const std::string& message : plan.ReadinessMessages)
		{
			if (!message.empty())
			{
				return QString::fromStdString(message);
			}
		}

		return "This workflow is currently blocked.";
	}

	static QString FirstBlockingReadinessMessage(const std::vector<std::string>& readinessMessages)
	{
		for (const std::string& message : readinessMessages)
		{
			if (!message.empty())
			{
				return QString::fromStdString(message);
			}
		}

		return "This workflow is currently blocked.";
	}

	LauncherMainWindow::LauncherMainWindow(
	    std::filesystem::path repositoryRoot,
	    LauncherContentModel& contentModel,
	    LauncherSettings& settings,
	    LauncherBackend& backend,
	    QWidget* parent) :
	    QMainWindow(parent),
	    m_repositoryRoot(std::move(repositoryRoot)),
	    m_contentModel(contentModel),
	    m_settings(settings),
	    m_backend(backend)
	{
		setWindowTitle("Sparkle Launcher");
		setMinimumSize(kLauncherMinimumWidth, kLauncherMinimumHeight);
		resize(kLauncherInitialWidth, kLauncherInitialHeight);
		m_icons.Load();
		const QIcon applicationIcon = m_icons.ApplicationIcon();
		QGuiApplication::setWindowIcon(applicationIcon);
		setWindowIcon(applicationIcon);

		QWidget* centralWidget = new QWidget(this);
		QVBoxLayout* rootLayout = new QVBoxLayout(centralWidget);
		rootLayout->setContentsMargins(0, 0, 0, 0);
		rootLayout->setSpacing(0);

		m_workflowPanel = new LauncherWorkflowPanel(
		    m_icons,
		    [this](const QString& operationId) { return DisplayNameForOperation(operationId); },
		    [this](QWidget* widget) { RegisterFocusable(widget); },
		    [this](QWidget* parent) { return CreateOptionsPanel(parent); },
		    centralWidget);
		connect(m_workflowPanel, &LauncherWorkflowPanel::OperationSelected, this, &LauncherMainWindow::SetSelectedOperation);
		rootLayout->addWidget(m_workflowPanel, 1);
		m_activityPanel = new LauncherActivityPanel(m_icons, [this](QWidget* widget) { RegisterFocusable(widget); }, centralWidget);
		rootLayout->addWidget(m_activityPanel, 0);
		rootLayout->addWidget(CreateFooterContextPanel(centralWidget), 0);
		RefreshContextSelectors();
		setCentralWidget(centralWidget);
		const QString initialOperationId = m_workflowPanel->InitialOperationId();
		if (!initialOperationId.isEmpty())
		{
			SetSelectedOperation(initialOperationId);
		}

		ConfigureTabOrder();
		ApplyVisualStyle();
		ApplyNativeDarkTitleBar(*this);

		connect(&m_contentModel, &LauncherContentModel::ContentChanged, this, [this]() { ScheduleUiRefresh(false); });
		connect(&m_contentModel, &LauncherContentModel::ContentDiscoveryFailed, this, &LauncherMainWindow::SetStartupNotice);
		connect(&m_settings, &LauncherSettings::SettingsChanged, this, [this]() { ScheduleUiRefresh(false); });
		connect(&m_backend, &LauncherBackend::OperationStarted, this, &LauncherMainWindow::DisplayOperationStarted);
		connect(&m_backend, &LauncherBackend::OperationOutputReceived, this, &LauncherMainWindow::AppendOperationOutput);
		connect(&m_backend, &LauncherBackend::OperationFinished, this, &LauncherMainWindow::DisplayOperationFinished);
		connect(qApp, &QGuiApplication::applicationStateChanged, this, &LauncherMainWindow::HandleApplicationStateChanged);

		QTimer::singleShot(0, this, &LauncherMainWindow::RefreshContent);
	}

	void LauncherMainWindow::HandleApplicationStateChanged(Qt::ApplicationState state)
	{
		if (state != Qt::ApplicationActive)
		{
			return;
		}

		const qint64 nowMs = QDateTime::currentMSecsSinceEpoch();
		if (m_lastActivationRefreshMs != 0 && nowMs - m_lastActivationRefreshMs < kActivationRefreshIntervalMs)
		{
			return;
		}

		m_lastActivationRefreshMs = nowMs;
		ScheduleUiRefresh(true);
	}

	void LauncherMainWindow::SetStartupNotice(const QString& message)
	{
		if (!message.isEmpty())
		{
			UpdateRunAvailability();
		}
	}

	void LauncherMainWindow::RefreshContent()
	{
		ScheduleUiRefresh(true);
	}

	void LauncherMainWindow::ScheduleUiRefresh(bool refreshContent)
	{
		m_refreshContentRequested = m_refreshContentRequested || refreshContent;
		if (m_isApplyingUiRefresh || m_uiRefreshQueued)
		{
			return;
		}

		m_uiRefreshQueued = true;
		QTimer::singleShot(0, this, &LauncherMainWindow::ApplyScheduledUiRefresh);
	}

	void LauncherMainWindow::ApplyScheduledUiRefresh()
	{
		if (m_isApplyingUiRefresh)
		{
			return;
		}

		m_uiRefreshQueued = false;
		const bool refreshContent = m_refreshContentRequested;
		m_refreshContentRequested = false;
		m_isApplyingUiRefresh = true;

		if (refreshContent)
		{
			m_contentModel.Refresh(m_repositoryRoot);
			RefreshContextSelectors();
		}

		RebuildOptionsPages();
		UpdateRunAvailability();
		m_isApplyingUiRefresh = false;
		if (m_refreshContentRequested)
		{
			ScheduleUiRefresh(false);
		}
	}

	void LauncherMainWindow::SetControlsEnabled(bool enabled)
	{
		if (m_cleanButton != nullptr)
		{
			m_cleanButton->setEnabled(enabled);
		}
		if (m_runButton != nullptr)
		{
			m_runButton->setEnabled(enabled);
		}
		if (m_optionsStack != nullptr)
		{
			m_optionsStack->setVisible(enabled);
		}
	}

	void LauncherMainWindow::RebuildOptionsPages()
	{
		if (m_optionsStack == nullptr || m_isRebuildingOptions)
		{
			return;
		}

		int preservedVerticalScroll = 0;
		int preservedHorizontalScroll = 0;
		if (QScrollArea* currentScrollArea = qobject_cast<QScrollArea*>(m_optionsStack->currentWidget()))
		{
			if (QScrollBar* verticalScrollBar = currentScrollArea->verticalScrollBar())
			{
				preservedVerticalScroll = verticalScrollBar->value();
			}
			if (QScrollBar* horizontalScrollBar = currentScrollArea->horizontalScrollBar())
			{
				preservedHorizontalScroll = horizontalScrollBar->value();
			}
		}

		m_isRebuildingOptions = true;

		while (m_optionsStack->count() > 0)
		{
			QWidget* page = m_optionsStack->widget(0);
			m_optionsStack->removeWidget(page);
			page->deleteLater();
		}
		m_optionsPageByOperation.clear();
		m_levelStatusLabels.clear();
		m_levelActionButtons.clear();
		m_sourceDependencyStatusLabels.clear();
		m_sourceDependencyActionButtons.clear();

		EnsureOptionsPage(m_selectedOperationId);
		if (m_optionsPageByOperation.contains(m_selectedOperationId))
		{
			m_optionsStack->setCurrentIndex(m_optionsPageByOperation.value(m_selectedOperationId));
			if (QScrollArea* rebuiltScrollArea = qobject_cast<QScrollArea*>(m_optionsStack->currentWidget()))
			{
				if (QScrollBar* verticalScrollBar = rebuiltScrollArea->verticalScrollBar())
				{
					QPointer<QScrollBar> guardedScrollBar(verticalScrollBar);
					QTimer::singleShot(
					    0,
					    this,
					    [guardedScrollBar, preservedVerticalScroll]()
					    {
						    if (guardedScrollBar != nullptr)
						    {
							    guardedScrollBar->setValue(preservedVerticalScroll);
						    }
					    });
				}
				if (QScrollBar* horizontalScrollBar = rebuiltScrollArea->horizontalScrollBar())
				{
					QPointer<QScrollBar> guardedScrollBar(horizontalScrollBar);
					QTimer::singleShot(
					    0,
					    this,
					    [guardedScrollBar, preservedHorizontalScroll]()
					    {
						    if (guardedScrollBar != nullptr)
						    {
							    guardedScrollBar->setValue(preservedHorizontalScroll);
						    }
					    });
				}
			}
		}

		m_isRebuildingOptions = false;
	}

	void LauncherMainWindow::EnsureOptionsPage(const QString& operationId)
	{
		if (m_optionsStack == nullptr || operationId.isEmpty() || m_optionsPageByOperation.contains(operationId))
		{
			return;
		}

		const int pageIndex = m_optionsStack->addWidget(CreateOptionsPage(operationId, m_optionsStack));
		m_optionsPageByOperation.insert(operationId, pageIndex);
		m_optionsStack->setCurrentIndex(pageIndex);
	}

	void LauncherMainWindow::RegisterFocusable(QWidget* widget)
	{
		if (widget == nullptr)
		{
			return;
		}

		widget->setFocusPolicy(Qt::StrongFocus);
		m_tabOrderWidgets.push_back(widget);
	}

	void LauncherMainWindow::ConfigureTabOrder()
	{
		QWidget* previousWidget = nullptr;
		for (QWidget* widget : m_tabOrderWidgets)
		{
			if (widget == nullptr)
			{
				continue;
			}

			if (previousWidget != nullptr)
			{
				setTabOrder(previousWidget, widget);
			}
			previousWidget = widget;
		}
	}

	void LauncherMainWindow::UpdateRunAvailability()
	{
		if (m_runButton == nullptr || m_cleanButton == nullptr)
		{
			return;
		}

		if (m_selectedOperationId.isEmpty())
		{
			const QString reason = "Select a workflow before running.";
			m_runButton->setEnabled(false);
			m_runButton->setToolTip(reason);
			m_runButton->setAccessibleDescription(reason);
			m_cleanButton->setEnabled(false);
			m_cleanButton->setToolTip("Select a workflow before cleaning generated outputs.");
			m_cleanButton->setAccessibleDescription("Select a workflow before cleaning generated outputs.");
			return;
		}

		m_runButton->setVisible(true);
		const bool isLevelCatalog = m_selectedOperationId == LauncherHomeOperationId();
		m_cleanButton->setVisible(
		    isLevelCatalog || m_selectedOperationId == "workspace.clean" || SupportsActionSpecificClean(m_selectedOperationId));

		if (OperationNeedsContent(m_selectedOperationId) && m_contentModel.ContentId().isEmpty())
		{
			const QString reason =
			    "Repository content is unavailable. Confirm this is a complete Sparkle workspace, then regenerate build files if needed.";
			m_runButton->setEnabled(false);
			m_runButton->setToolTip(reason);
			m_runButton->setAccessibleDescription(reason);
			m_cleanButton->setEnabled(false);
			m_cleanButton->setToolTip("Repository content is unavailable for this clean action.");
			m_cleanButton->setAccessibleDescription("Repository content is unavailable for this clean action.");
			return;
		}

		const QVector<LauncherCleanTarget> cleanTargets = SupportsActionSpecificClean(m_selectedOperationId)
		    ? BuildActionSpecificCleanTargets(BuildActionCleanTargetContext(
		          m_repositoryRoot,
		          m_contentModel,
		          m_settings,
		          std::filesystem::path(QCoreApplication::applicationFilePath().toStdString()),
		          m_selectedOperationId))
		    : QVector<LauncherCleanTarget>();
		const bool cleanWorkspaceSelected = m_selectedOperationId == "workspace.clean";
		bool canSyncLevel = false;
		bool hasSelectedLevels = false;
		bool hasExtractedLevelContent = false;
		if (isLevelCatalog)
		{
			const LauncherLevelUiModel levelModel = BuildLevelUiModel();
			for (const LauncherLevelUiEntry& level : levelModel.Levels)
			{
				canSyncLevel = canSyncLevel || (level.Id != "Empty" && level.CanSelect);
				if (level.Id != "Empty" && level.Selected)
				{
					hasSelectedLevels = true;
					break;
				}
			}

			const LauncherContentSummary* content = m_contentModel.Content();
			if (content != nullptr)
			{
				try
				{
					hasExtractedLevelContent = !BuildLevelCleanTargets(*content).empty();
				}
				catch (const Diagnostics::Error&)
				{
					// The catalog page owns the detailed load diagnostic. Keep the footer safely disabled here.
				}
			}
		}
		const bool canClean = cleanWorkspaceSelected || hasSelectedLevels || hasExtractedLevelContent || !cleanTargets.isEmpty();
		m_cleanButton->setEnabled(canClean);
		m_cleanButton->setToolTip(
		    isLevelCatalog
		        ? (canClean ? "Disable all selected levels and clean extracted external level content. Cached archives are preserved."
		                    : "No selected levels are available to clean.")
		        : cleanWorkspaceSelected
		        ? "Clean all generated repository state."
		        : (canClean ? "Clean only the generated outputs tied to " + DisplayNameForOperation(m_selectedOperationId) + "."
		                    : "Clean is not available for this workflow."));
		m_cleanButton->setAccessibleDescription(m_cleanButton->toolTip());
		if (isLevelCatalog)
		{
			const QString reason = canSyncLevel ? QStringLiteral("Sync all available levels. Existing runs keep going.")
			                                    : QStringLiteral("No catalog levels are available to sync.");
			m_runButton->setEnabled(canSyncLevel);
			m_runButton->setToolTip(reason);
			m_runButton->setAccessibleDescription(reason);
			return;
		}

		if (FindBuildWorkspaceOperationDefinition(m_selectedOperationId.toStdString()).has_value())
		{
			const BuildWorkspaceOperationRequest request = BuildWorkspacePlanRequest(m_repositoryRoot, m_contentModel, m_settings);
			const BuildWorkspaceOperationPlan plan = PlanBuildWorkspaceOperation(m_selectedOperationId.toStdString(), request);
			const QString reason = plan.CanRun ? "Run " + DisplayNameForOperation(m_selectedOperationId) + ". Existing runs keep going."
			                                   : FirstBlockingReadinessMessage(plan);
			m_runButton->setEnabled(plan.CanRun);
			m_runButton->setToolTip(reason);
			m_runButton->setAccessibleDescription(reason);
			return;
		}

		if (FindCookOperationDefinition(m_selectedOperationId.toStdString()).has_value())
		{
			const LauncherOperationRequest operationRequest =
			    BuildLauncherOperationRequest(m_repositoryRoot, m_contentModel, m_settings, m_selectedOperationId);
			const CookOperationPlan plan =
			    PlanCookOperation(m_selectedOperationId.toStdString(), LauncherOperationRequestMapping::Cook(operationRequest));
			const QString reason = plan.CanRun
			    ? "Run " + DisplayNameForOperation(m_selectedOperationId) + ". Existing runs keep going."
			    : (plan.ReadinessMessages.empty() ? QStringLiteral("This cooking workflow is currently blocked.")
			                                      : QString::fromStdString(plan.ReadinessMessages.back()));
			m_runButton->setEnabled(plan.CanRun);
			m_runButton->setToolTip(reason);
			m_runButton->setAccessibleDescription(reason);
			return;
		}

		const QString title = DisplayNameForOperation(m_selectedOperationId);
		const QString actionDescription = "Run " + title + ". Existing runs keep going.";
		m_runButton->setEnabled(true);
		m_runButton->setToolTip(actionDescription);
		m_runButton->setAccessibleDescription(actionDescription);
	}

	void LauncherMainWindow::SetSelectedOperation(const QString& operationId)
	{
		m_selectedOperationId = operationId;
		SetControlsEnabled(true);
		if (m_actionMetaPanel != nullptr)
		{
			m_actionMetaPanel->setVisible(true);
		}
		if (m_runButton != nullptr)
		{
			const bool destructive = operationId == "workspace.clean";
			LauncherIcon actionIcon = LauncherIcon::Run;
			if (operationId == LauncherHomeOperationId() || operationId.startsWith("workspace.sync"))
			{
				actionIcon = LauncherIcon::Sync;
			}
			else if (operationId.startsWith("workspace.build") || operationId == "launcher.build.self")
			{
				actionIcon = LauncherIcon::Build;
			}
			else if (operationId.startsWith("cook."))
			{
				actionIcon = LauncherIcon::Cook;
			}
			else if (destructive)
			{
				actionIcon = LauncherIcon::Clean;
			}
			m_runButton->setProperty("ActionTone", destructive ? "destructive" : "primary");
			m_runButton->setIcon(m_icons.Icon(actionIcon, QColor(destructive ? "#ffffff" : "#071006")));
			m_runButton->setText(
			    operationId == LauncherHomeOperationId() ? "Sync All"
			        : operationId == "workspace.clean"   ? "Clean Selected"
			                                             : PrimaryActionLabelForOperationId(operationId));
			m_runButton->setAccessibleName(
			    operationId == LauncherHomeOperationId() ? "Sync all available levels"
			        : operationId == "workspace.clean"   ? "Clean selected generated repository state"
			                                             : "Run selected workflow");
			m_runButton->style()->unpolish(m_runButton);
			m_runButton->style()->polish(m_runButton);
		}
		if (m_cleanButton != nullptr)
		{
			const bool cleanAll = operationId == "workspace.clean" || operationId == LauncherHomeOperationId();
			m_cleanButton->setProperty("ActionTone", "destructive");
			m_cleanButton->setText(cleanAll ? "Clean All" : "Clean");
			m_cleanButton->setAccessibleName(
			    operationId == LauncherHomeOperationId() ? "Clean all selected levels"
			        : operationId == "workspace.clean"   ? "Clean all generated repository state"
			                                             : "Clean selected workflow outputs");
			m_cleanButton->style()->unpolish(m_cleanButton);
			m_cleanButton->style()->polish(m_cleanButton);
		}
		if (m_optionsStack != nullptr)
		{
			EnsureOptionsPage(operationId);
			if (m_optionsPageByOperation.contains(operationId))
			{
				m_optionsStack->setCurrentIndex(m_optionsPageByOperation.value(operationId));
			}
		}

		if (m_workflowPanel != nullptr)
		{
			m_workflowPanel->SetSelectedOperation(operationId);
		}

		UpdateRunAvailability();
	}

	void LauncherMainWindow::ApplyVisualStyle()
	{
		ApplyLauncherVisualStyle(*this);
	}
}
