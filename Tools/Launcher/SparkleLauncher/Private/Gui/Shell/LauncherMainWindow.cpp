#include "LauncherMainWindow.h"

#include "LauncherActionHistoryModel.h"
#include "LauncherActionWidgets.h"
#include "LauncherArtworkWidgets.h"
#include "LauncherBackend.h"
#include "LauncherCleanUiModel.h"
#include "LauncherDependencyUiModel.h"
#include "LauncherHomeWidgets.h"
#include "LauncherIconLibrary.h"
#include "LauncherLayoutWidgets.h"
#include "LauncherOperationRequestFactory.h"
#include "LauncherProjectModel.h"
#include "LauncherOutputWidgets.h"
#include "LauncherPrerequisitePrompts.h"
#include "LauncherRecoveryUiModel.h"
#include "LauncherSettings.h"
#include "LauncherToolchainUiModel.h"
#include "LauncherUiDesign.h"
#include "LauncherUiModel.h"
#include "LauncherVisualStyle.h"
#include "LauncherWorkflowCatalog.h"

#include "SparkleLauncher/BuildWorkspaceOperations.h"
#include "SparkleLauncher/CookOperations.h"
#include "SparkleLauncher/LauncherPaths.h"
#include "SparkleLauncher/LaunchOperations.h"
#include "SparkleLauncher/MaintenanceOperations.h"

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
#include <QtWidgets/QAbstractButton>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QScrollArea>
#include <QtWidgets/QScrollBar>
#include <QtWidgets/QSizePolicy>
#include <QtWidgets/QStyle>
#include <QtWidgets/QToolButton>
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
	static constexpr int kMaxOperationOutputCharacters = 1000000;
	static constexpr int kSpaceTiny = LauncherUi::Space::Tiny;
	static constexpr int kSpaceSmall = LauncherUi::Space::Small;
	static constexpr int kSpaceMedium = LauncherUi::Space::Medium;
	static constexpr int kSpaceLarge = LauncherUi::Space::Large;
	static constexpr int kPanelHorizontalMargin = LauncherUi::Shell::PanelHorizontalMargin;
	static constexpr int kPanelVerticalMargin = LauncherUi::Shell::PanelVerticalMargin;
	static constexpr int kWorkflowRailWidth = LauncherUi::Shell::RailWidth;
	static constexpr int kWorkflowGroupMinHeight = LauncherUi::Shell::RailItemMinHeight;
	static constexpr int kWorkflowButtonMinHeight = LauncherUi::Shell::TabMinHeight;
	static constexpr int kFieldLabelWidth = LauncherUi::Row::FieldLabelWidth;
	static constexpr int kOperationOutputMinHeight = LauncherUi::OperationOutput::MinHeight;
	static constexpr int kOperationOutputCompactMaxHeight = LauncherUi::OperationOutput::CompactMaxHeight;
	static constexpr int kOperationOutputProminentMinHeight = LauncherUi::OperationOutput::ProminentMinHeight;
	static constexpr int kOperationOutputMaxHeight = LauncherUi::OperationOutput::MaxHeight;
	static constexpr int kActivityPanelCollapsedHeight = LauncherUi::Activity::CollapsedHeight;
	static constexpr int kActivityPanelExpandedHeight = LauncherUi::Activity::ExpandedHeight;
	static constexpr int kLauncherIconSize = LauncherUi::Icon::DefaultSize;
	static constexpr int kLauncherMinimumWidth = LauncherUi::Window::MinimumWidth;
	static constexpr int kLauncherMinimumHeight = LauncherUi::Window::MinimumHeight;
	static constexpr int kLauncherInitialWidth = LauncherUi::Window::InitialWidth;
	static constexpr int kLauncherInitialHeight = LauncherUi::Window::InitialHeight;
	static constexpr int kStatusChipColumnWidth = LauncherUi::Row::StatusChipColumnWidth;
	static constexpr int kStatusActionColumnWidth = LauncherUi::Row::StatusActionColumnWidth;
	static constexpr const char* kColorStateQueued = LauncherUi::Color::StateQueued;
	static constexpr const char* kColorStateRunning = LauncherUi::Color::StateRunning;
	static constexpr const char* kColorStateSuccess = LauncherUi::Color::StateSuccess;
	static constexpr const char* kColorStateDestructive = LauncherUi::Color::StateDestructive;
	static constexpr const char* kColorStateWarning = LauncherUi::Color::StateWarning;
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

	static bool ReadinessContains(const std::vector<std::string>& readinessMessages, const QString& phrase)
	{
		for (const std::string& message : readinessMessages)
		{
			if (QString::fromStdString(message).contains(phrase, Qt::CaseInsensitive))
			{
				return true;
			}
		}
		return false;
	}

	LauncherMainWindow::LauncherMainWindow(
	    std::filesystem::path repositoryRoot,
	    LauncherProjectModel& projectModel,
	    LauncherSettings& settings,
	    LauncherBackend& backend,
	    QWidget* parent)
	    : QMainWindow(parent)
	    , m_repositoryRoot(std::move(repositoryRoot))
	    , m_projectModel(projectModel)
	    , m_settings(settings)
	    , m_backend(backend)
	{
		m_actionHistory.Load(m_repositoryRoot);
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

		rootLayout->addWidget(CreateWorkflowSurface(), 1);
		rootLayout->addWidget(CreateOutputPanel(), 0);
		setCentralWidget(centralWidget);
		const QVector<LauncherWorkflowDefinition> workflows = CreateLauncherWorkflowCatalog();
		if (!workflows.empty() && !workflows.front().OperationIds.empty())
		{
			SetSelectedOperation(workflows.front().OperationIds.front());
		}

		ConfigureTabOrder();
		ApplyVisualStyle();
		ApplyNativeDarkTitleBar(*this);

		connect(&m_projectModel, &LauncherProjectModel::ProjectsChanged, this, &LauncherMainWindow::PopulateProjectSelectors);
		connect(&m_projectModel, &LauncherProjectModel::SelectionChanged, this, [this](const QString&) {
			PopulateProjectSelectors();
			RebuildOptionsPages();
			UpdateRunAvailability();
		});
		connect(&m_projectModel, &LauncherProjectModel::ProjectDiscoveryFailed, this, &LauncherMainWindow::SetStartupNotice);
		connect(&m_settings, &LauncherSettings::SettingsChanged, this, [this]() {
			RebuildOptionsPages();
			UpdateRunAvailability();
		});
		connect(&m_backend, &LauncherBackend::OperationStarted, this, &LauncherMainWindow::DisplayOperationStarted);
		connect(&m_backend, &LauncherBackend::OperationOutputReceived, this, &LauncherMainWindow::AppendOperationOutput);
		connect(&m_backend, &LauncherBackend::OperationFinished, this, &LauncherMainWindow::DisplayOperationFinished);

		UpdateProgress();
		QTimer::singleShot(0, this, &LauncherMainWindow::RefreshProjects);
	}

	void LauncherMainWindow::SetStartupNotice(const QString& message)
	{
		if (!message.isEmpty())
		{
			UpdateRunAvailability();
		}
	}

	void LauncherMainWindow::RefreshProjects()
	{
		m_projectModel.Refresh(m_repositoryRoot);
	}

	void LauncherMainWindow::SelectWorkflowGroupButton(QAbstractButton* button)
	{
		if (button == nullptr || m_operationStack == nullptr)
		{
			return;
		}

		const int workflowIndex = button->property("WorkflowIndex").toInt();
		if (workflowIndex >= 0 && workflowIndex < m_operationStack->count())
		{
			m_operationStack->setCurrentIndex(workflowIndex);
			SetActiveWorkflowGroup(workflowIndex);

			const QVector<LauncherWorkflowDefinition> workflows = CreateLauncherWorkflowCatalog();
			if (workflowIndex < workflows.size() && !workflows[workflowIndex].OperationIds.empty())
			{
				m_operationStack->setVisible(workflows[workflowIndex].OperationIds.size() > 1);
				const QString lastOperationId = m_lastOperationByWorkflowIndex.value(workflowIndex);
				SetSelectedOperation(workflows[workflowIndex].OperationIds.contains(lastOperationId) ? lastOperationId : workflows[workflowIndex].OperationIds.front());
			}
		}
	}

	void LauncherMainWindow::SelectProcessButton(QAbstractButton* button)
	{
		if (button == nullptr)
		{
			return;
		}

		SetSelectedOperation(button->property("OperationId").toString());
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

		EnsureOptionsPage(m_selectedOperationId);
		if (m_optionsPageByOperation.contains(m_selectedOperationId))
		{
			m_optionsStack->setCurrentIndex(m_optionsPageByOperation.value(m_selectedOperationId));
			if (QScrollArea* rebuiltScrollArea = qobject_cast<QScrollArea*>(m_optionsStack->currentWidget()))
			{
				if (QScrollBar* verticalScrollBar = rebuiltScrollArea->verticalScrollBar())
				{
					verticalScrollBar->setValue(preservedVerticalScroll);
				}
				if (QScrollBar* horizontalScrollBar = rebuiltScrollArea->horizontalScrollBar())
				{
					horizontalScrollBar->setValue(preservedHorizontalScroll);
				}
			}
		}

		m_projectSelectors.clear();
		for (QComboBox* combo : findChildren<QComboBox*>())
		{
			if (combo != nullptr && combo->property("ProjectSelector").toBool())
			{
				m_projectSelectors.push_back(combo);
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

	QIcon LauncherMainWindow::WorkflowIconForKey(const QString& iconKey) const
	{
		if (iconKey == "home")
		{
			return m_icons.Icon(LauncherIcon::Start, QColor(kColorStateQueued));
		}
		if (iconKey == "launch")
		{
			return m_icons.Icon(LauncherIcon::Run, QColor(kColorStateQueued));
		}
		if (iconKey == "sync")
		{
			return m_icons.Icon(LauncherIcon::Sync, QColor(kColorStateQueued));
		}
		if (iconKey == "build")
		{
			return m_icons.Icon(LauncherIcon::Build, QColor(kColorStateQueued));
		}
		if (iconKey == "cook")
		{
			return m_icons.Icon(LauncherIcon::Cook, QColor(kColorStateQueued));
		}
		if (iconKey == "test")
		{
			return m_icons.Icon(LauncherIcon::Done, QColor(kColorStateQueued));
		}
		if (iconKey == "package")
		{
			return m_icons.Icon(LauncherIcon::Package, QColor(kColorStateQueued));
		}
		if (iconKey == "maintain")
		{
			return m_icons.Icon(LauncherIcon::Maintain, QColor(kColorStateQueued));
		}
		return {};
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

	void LauncherMainWindow::SetActiveWorkflowGroup(int workflowIndex)
	{
		if (m_workflowGroupButtonGroup == nullptr)
		{
			return;
		}

		for (QAbstractButton* button : m_workflowGroupButtonGroup->buttons())
		{
			const bool active = button != nullptr && button->property("WorkflowIndex").toInt() == workflowIndex;
			button->setProperty("ActiveState", active ? "true" : "false");
			button->style()->unpolish(button);
			button->style()->polish(button);
			button->update();
		}
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

		if (m_selectedOperationId == LauncherHomeOperationId())
		{
			const QString reason = "Use Quick Start cards for the next best action.";
			m_cleanButton->setVisible(false);
			m_cleanButton->setEnabled(false);
			m_cleanButton->setToolTip("Home summarizes readiness and does not own generated outputs.");
			m_cleanButton->setAccessibleDescription(m_cleanButton->toolTip());
			m_runButton->setVisible(false);
			m_runButton->setEnabled(false);
			m_runButton->setToolTip(reason);
			m_runButton->setAccessibleDescription(reason);
			return;
		}

		m_runButton->setVisible(true);
		m_cleanButton->setVisible(m_selectedOperationId == "workspace.clean" || SupportsActionSpecificClean(m_selectedOperationId));

		if (OperationNeedsProject(m_selectedOperationId) && m_projectModel.SelectedProjectId().isEmpty())
		{
			const QString reason = "No project discovered. Confirm this is a Sparkle repository or package root with Projects/<Project> markers, then run Generate Build Files if rebuilding from source.";
			m_runButton->setEnabled(false);
			m_runButton->setToolTip(reason);
			m_runButton->setAccessibleDescription(reason);
			m_cleanButton->setEnabled(false);
			m_cleanButton->setToolTip("No project discovered for this clean action.");
			m_cleanButton->setAccessibleDescription("No project discovered for this clean action.");
			return;
		}

		const QVector<LauncherCleanTarget> cleanTargets =
		    SupportsActionSpecificClean(m_selectedOperationId) ?
		        BuildActionSpecificCleanTargets(BuildActionCleanTargetContext(
		            m_repositoryRoot,
		            m_projectModel,
		            m_settings,
		            std::filesystem::path(QCoreApplication::applicationFilePath().toStdString()),
		            m_selectedOperationId)) :
		        QVector<LauncherCleanTarget>();
		const bool cleanWorkspaceSelected = m_selectedOperationId == "workspace.clean";
		const bool canClean = cleanWorkspaceSelected || !cleanTargets.isEmpty();
		m_cleanButton->setEnabled(canClean);
		m_cleanButton->setToolTip(
		    cleanWorkspaceSelected ?
		        "Clean all generated repository state." :
		        (canClean ? "Clean only the generated outputs tied to " + DisplayNameForOperation(m_selectedOperationId) + "." : "Clean is not available for this workflow."));
		m_cleanButton->setAccessibleDescription(m_cleanButton->toolTip());

		if (FindBuildWorkspaceOperationDefinition(m_selectedOperationId.toStdString()).has_value())
		{
			const BuildWorkspaceOperationRequest request = BuildWorkspacePlanRequest(m_repositoryRoot, m_projectModel, m_settings);
			const BuildWorkspaceOperationPlan plan = PlanBuildWorkspaceOperation(m_selectedOperationId.toStdString(), request);
			const QString reason = plan.CanRun ? "Run " + DisplayNameForOperation(m_selectedOperationId) + ". Existing runs keep going." : FirstBlockingReadinessMessage(plan);
			const bool canRetryWorkspacePrerequisite =
			    !plan.CanRun && plan.Toolchain.RequiredToolsAvailable &&
			    (ReadinessContains(plan.ReadinessMessages, "Generated build files are not current") ||
			     ReadinessContains(plan.ReadinessMessages, "Run Generate Build Files first") || ReadinessContains(plan.ReadinessMessages, "not current"));
			m_runButton->setEnabled(plan.CanRun || canRetryWorkspacePrerequisite);
			m_runButton->setToolTip(reason);
			m_runButton->setAccessibleDescription(reason);
			return;
		}

		if (m_selectedOperationId == "quality.format")
		{
			MaintenanceOperationRequest request;
			request.RepositoryRoot = m_repositoryRoot;
			request.ProjectId = m_projectModel.ActiveProjectId().toStdString();
			request.EditorProfile = m_settings.EditorProfile().toStdString();
			request.RequestedFormatMode = m_settings.FormatMode() == "check" ? FormatMode::Check : FormatMode::Apply;
			const MaintenanceOperationPlan plan = PlanMaintenanceOperation(m_selectedOperationId.toStdString(), request);
			const QString reason = plan.CanRun ? "Run " + DisplayNameForOperation(m_selectedOperationId) + ". Existing runs keep going." : FirstBlockingReadinessMessage(plan.ReadinessMessages);
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
		const QString operationTitle = DisplayNameForOperation(operationId);
		QString workflowTitle = operationTitle;
		const QVector<LauncherWorkflowDefinition> workflows = CreateLauncherWorkflowCatalog();
		if (m_workflowPageByOperation.contains(operationId))
		{
			const int workflowIndex = m_workflowPageByOperation.value(operationId);
			if (workflowIndex >= 0 && workflowIndex < workflows.size())
			{
				workflowTitle = workflows[workflowIndex].Title;
			}
		}
		SetControlsEnabled(true);
		UpdateActionHistoryDisplay();
		const bool isStaticPage = operationId == LauncherHomeOperationId();
		if (m_activeOperationLabel != nullptr)
		{
			m_activeOperationLabel->setText(workflowTitle);
			m_activeOperationLabel->setVisible(true);
		}
		if (m_actionMetaPanel != nullptr)
		{
			m_actionMetaPanel->setVisible(!isStaticPage);
		}
		if (m_runButton != nullptr)
		{
			m_runButton->setText(operationId == "workspace.clean" ? "Clean Selected" : PrimaryActionLabelForOperationId(operationId));
		}
		if (m_cleanButton != nullptr)
		{
			m_cleanButton->setText(operationId == "workspace.clean" ? "Clean All" : "Clean");
			m_cleanButton->setAccessibleName(operationId == "workspace.clean" ? "Clean all generated repository state" : "Clean selected workflow outputs");
		}
		if (m_optionsStack != nullptr)
		{
			EnsureOptionsPage(operationId);
			if (m_optionsPageByOperation.contains(operationId))
			{
				m_optionsStack->setCurrentIndex(m_optionsPageByOperation.value(operationId));
			}
		}

		if (m_processButtonGroup != nullptr)
		{
			for (QAbstractButton* button : m_processButtonGroup->buttons())
			{
				button->setChecked(button->property("OperationId").toString() == operationId);
			}
		}

		if (m_operationStack != nullptr && m_workflowPageByOperation.contains(operationId))
		{
			const int workflowIndex = m_workflowPageByOperation.value(operationId);
			m_lastOperationByWorkflowIndex.insert(workflowIndex, operationId);
			m_operationStack->setCurrentIndex(workflowIndex);
			SetActiveWorkflowGroup(workflowIndex);
			if (workflowIndex >= 0 && workflowIndex < workflows.size())
			{
				m_operationStack->setVisible(workflows[workflowIndex].OperationIds.size() > 1);
			}
		}

		UpdateRunAvailability();
	}

	void LauncherMainWindow::PopulateProjectSelectors()
	{
		for (QComboBox* combo : m_projectSelectors)
		{
			if (combo != nullptr)
			{
				PopulateProjectCombo(*combo);
			}
		}
	}

	void LauncherMainWindow::PopulateProjectCombo(QComboBox& combo) const
	{
		const QSignalBlocker blocker(&combo);
		combo.clear();
		if (m_projectModel.Projects().empty())
		{
			combo.addItem("No projects found", "");
			combo.setToolTip("No projects were discovered. Confirm the selected root contains Projects/<Project> markers or inspect project discovery output.");
			combo.setEnabled(false);
			return;
		}

		combo.setEnabled(true);
		combo.setToolTip("Project used by this workflow.");
		for (const LauncherProjectSummary& project : m_projectModel.Projects())
		{
			combo.addItem(project.DisplayName, project.Id);
			combo.setItemData(combo.count() - 1, project.Id + "\n" + QString::fromStdString(project.RootPath.string()), Qt::ToolTipRole);
		}

		const int selectedIndex = combo.findData(m_projectModel.SelectedProjectId());
		combo.setCurrentIndex(selectedIndex >= 0 ? selectedIndex : 0);
	}

	void LauncherMainWindow::ApplyVisualStyle()
	{
		ApplyLauncherVisualStyle(*this);
	}
}
