#include "LauncherMainWindow.h"

#include "LauncherBackend.h"
#include "LauncherProjectModel.h"
#include "LauncherSettings.h"

#include "SparkleLauncher/BuildWorkspaceOperations.h"
#include "SparkleLauncher/CookOperations.h"
#include "SparkleLauncher/LauncherPaths.h"
#include "SparkleLauncher/LaunchOperations.h"

#include <QtCore/QSignalBlocker>
#include <QtCore/QCoreApplication>
#include <QtCore/QProcess>
#include <QtCore/QRegularExpression>
#include <QtCore/QStringList>
#include <QtCore/QTimer>
#include <QtCore/Qt>
#include <QtGui/QBrush>
#include <QtGui/QClipboard>
#include <QtGui/QColor>
#include <QtGui/QFont>
#include <QtGui/QFontDatabase>
#include <QtGui/QGuiApplication>
#include <QtGui/QKeySequence>
#include <QtGui/QPainter>
#include <QtGui/QPixmap>
#include <QtGui/QTextCursor>
#include <QtGui/QTextDocument>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QScrollArea>
#include <QtWidgets/QStyle>
#include <QtWidgets/QWidget>

#include <array>
#include <cstdint>
#include <system_error>
#include <utility>

namespace SparkleLauncher
{
	static constexpr int kMaxOperationOutputCharacters = 1000000;
	static constexpr int kSpaceTiny = 2;
	static constexpr int kSpaceSmall = 8;
	static constexpr int kSpaceMedium = 12;
	static constexpr int kSpaceLarge = 16;
	static constexpr int kPanelHorizontalMargin = 18;
	static constexpr int kPanelVerticalMargin = 14;
	static constexpr int kWorkflowRailWidth = 332;
	static constexpr int kWorkflowGroupMinHeight = 30;
	static constexpr int kWorkflowButtonMinHeight = 32;
	static constexpr int kFieldLabelWidth = 116;
	static constexpr int kActivityListWidth = 260;
	static constexpr int kActivityListMaxHeight = 146;
	static constexpr int kOperationOutputMinHeight = 96;
	static constexpr int kOperationOutputCompactMaxHeight = 128;
	static constexpr int kOperationOutputProminentMinHeight = 136;
	static constexpr int kOperationOutputMaxHeight = 220;
	static constexpr int kLauncherIconSize = 14;
	static constexpr const char* kColorStateQueued = "#8b949e";
	static constexpr const char* kColorStateRunning = "#58a6ff";
	static constexpr const char* kColorStateSuccess = "#7ee787";
	static constexpr const char* kColorStateDestructive = "#ff7b72";
	static constexpr const char* kColorStateWarning = "#ffb454";

	struct CleanScopeUiOption
	{
		QString Label;
		QString Value;
		QString Detail;
		QString Preview;
		QString Group;
	};

	static QString ToDisplayPath(const std::filesystem::path& repositoryRoot, const std::filesystem::path& path)
	{
		std::error_code errorCode;
		const std::filesystem::path relative = std::filesystem::relative(path, repositoryRoot, errorCode);
		return QString::fromStdString((!errorCode && !relative.empty()) ? relative.generic_string() : path.generic_string());
	}

	static QString FormatDirectoryInventory(const std::filesystem::path& path)
	{
		std::error_code errorCode;
		if (!std::filesystem::exists(path, errorCode) || errorCode)
		{
			return "not present";
		}

		if (std::filesystem::is_regular_file(path, errorCode))
		{
			return "1 file";
		}

		std::uintmax_t fileCount = 0;
		std::uintmax_t directoryCount = 0;
		if (std::filesystem::is_directory(path, errorCode))
		{
			std::filesystem::recursive_directory_iterator iterator(
			    path,
			    std::filesystem::directory_options::skip_permission_denied,
			    errorCode);
			const std::filesystem::recursive_directory_iterator end;
			while (iterator != end)
			{
				const std::filesystem::directory_entry entry = *iterator;
				if (entry.is_directory(errorCode))
				{
					++directoryCount;
				}
				else if (entry.is_regular_file(errorCode))
				{
					++fileCount;
				}
				errorCode.clear();
				iterator.increment(errorCode);
				errorCode.clear();
			}
		}

		return QStringLiteral("%1 files, %2 folders").arg(fileCount).arg(directoryCount);
	}

	static std::filesystem::path ResolveCleanScopePreviewPath(const std::filesystem::path& repositoryRoot, const QString& projectId, const QString& scope)
	{
		if (scope == "selected-cooked")
		{
			return GetCookedProjectDirectory(repositoryRoot, projectId.toStdString());
		}
		if (scope == "all-cooked")
		{
			return GetBuildDirectory(repositoryRoot) / "Cooked";
		}
		if (scope == "build-tree")
		{
			return GetBuildDirectory(repositoryRoot);
		}
		if (scope == "shader-cache")
		{
			return GetBuildDirectory(repositoryRoot) / "Cache" / "Shaders";
		}
		if (scope == "deps")
		{
			return GetBuildDirectory(repositoryRoot) / "_deps";
		}
		if (scope == "logs")
		{
			return repositoryRoot / "logs";
		}
		return repositoryRoot;
	}

	static QString CleanScopeDisplayName(const QString& scopeValue)
	{
		if (scopeValue == "selected-cooked")
		{
			return "Project Cooked Data";
		}
		if (scopeValue == "all-cooked")
		{
			return "All Cooked Data";
		}
		if (scopeValue == "build-tree")
		{
			return "Build Artifacts";
		}
		if (scopeValue == "shader-cache")
		{
			return "Shader Cache";
		}
		if (scopeValue == "deps")
		{
			return "Third-Party Cache";
		}
		if (scopeValue == "logs")
		{
			return "Log Files";
		}
		if (scopeValue == "pristine")
		{
			return "Generated Workspace";
		}
		return scopeValue;
	}

	static QString FormatStatusPath(const std::filesystem::path& path)
	{
		return path.empty() ? QString() : QString::fromStdString(path.string());
	}

	static bool DirectoryHasEntries(const std::filesystem::path& path)
	{
		std::error_code errorCode;
		if (!std::filesystem::is_directory(path, errorCode))
		{
			return false;
		}
		return std::filesystem::directory_iterator(path, errorCode) != std::filesystem::directory_iterator();
	}

	static QString ToolchainStatusState(ToolchainItemState state, bool required)
	{
		switch (state)
		{
		case ToolchainItemState::Found:
			return "ok";
		case ToolchainItemState::Warning:
			return required ? "warning" : "neutral";
		case ToolchainItemState::Missing:
			return required ? "bad" : "neutral";
		}
		return "neutral";
	}

	static QString ToolchainStatusText(ToolchainItemState state, bool required)
	{
		switch (state)
		{
		case ToolchainItemState::Found:
			return "Ready";
		case ToolchainItemState::Warning:
			return required ? "Warning" : "Optional";
		case ToolchainItemState::Missing:
			return required ? "Missing" : "Optional";
		}
		return "Unknown";
	}

	static QString BuildGeneratorSummary(const BuildToolchainStatus& toolchain)
	{
		return QStringLiteral("Generator: %1 | Platform: %2%3")
		    .arg(QString::fromStdString(toolchain.Generator))
		    .arg(QString::fromStdString(toolchain.Platform))
		    .arg(toolchain.Toolset.empty() ? QString() : QStringLiteral(" | Toolset: %1").arg(QString::fromStdString(toolchain.Toolset)));
	}

	static QString RequiredToolProblemSummary(const BuildToolchainStatus& toolchain)
	{
		QStringList problems;
		for (const ToolchainItemStatus& item : toolchain.Items)
		{
			if (!item.Required || item.State == ToolchainItemState::Found)
			{
				continue;
			}
			problems.push_back(QString::fromStdString(item.DisplayName));
		}

		return problems.isEmpty() ? QString() : "Missing or blocked: " + problems.join(", ");
	}

	static QString CombineStatusDetail(const QString& first, const QString& second)
	{
		if (first.isEmpty())
		{
			return second;
		}
		if (second.isEmpty())
		{
			return first;
		}
		return first + " | " + second;
	}

	static WorkspaceIde SelectedWorkspaceIde(const LauncherSettings& settings)
	{
		WorkspaceIde ide = WorkspaceIde::VisualStudio;
		TryParseWorkspaceIde(settings.WorkspaceIde().toStdString(), ide);
		return ide;
	}

	static QString SelectedWorkspaceIdeName(const LauncherSettings& settings)
	{
		return QString::fromStdString(DisplayName(SelectedWorkspaceIde(settings)));
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
		setWindowTitle("Sparkle Launcher");
		setMinimumSize(980, 620);
		resize(1240, 800);
		LoadLauncherIconFont();
		const QIcon applicationIcon = CreateApplicationIcon();
		QGuiApplication::setWindowIcon(applicationIcon);
		setWindowIcon(applicationIcon);

		QWidget* centralWidget = new QWidget(this);
		QVBoxLayout* rootLayout = new QVBoxLayout(centralWidget);
		rootLayout->setContentsMargins(0, 0, 0, 0);
		rootLayout->setSpacing(0);

		QHBoxLayout* bodyLayout = new QHBoxLayout();
		bodyLayout->setContentsMargins(0, 0, 0, 0);
		bodyLayout->setSpacing(0);
		bodyLayout->addWidget(CreateWorkflowSurface(), 1);
		rootLayout->addLayout(bodyLayout, 1);
		rootLayout->addWidget(CreateOutputPanel());
		rootLayout->addWidget(CreateFooterContextPanel(centralWidget));
		setCentralWidget(centralWidget);
		const QVector<WorkflowDefinition> workflows = CreateWorkflowDefinitions();
		if (!workflows.empty() && !workflows.front().OperationIds.empty())
		{
			SetSelectedOperation(workflows.front().OperationIds.front());
		}

		ConfigureTabOrder();
		ApplyVisualStyle();

		connect(&m_projectModel, &LauncherProjectModel::ProjectsChanged, this, &LauncherMainWindow::PopulateProjectSelectors);
		connect(&m_projectModel, &LauncherProjectModel::SelectionChanged, this, &LauncherMainWindow::PopulateProjectSelectors);
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
			SetStatusMessage("Project discovery: " + message);
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

			const QVector<WorkflowDefinition> workflows = CreateWorkflowDefinitions();
			if (workflowIndex < workflows.size() && !workflows[workflowIndex].OperationIds.empty())
			{
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

	void LauncherMainWindow::DisplaySelectedRunOutput(QListWidgetItem* currentItem, QListWidgetItem*)
	{
		if (currentItem == nullptr)
		{
			return;
		}

		ShowRunOutput(currentItem->data(Qt::UserRole).toString());
	}

	void LauncherMainWindow::CopySelectedRunOutput()
	{
		if (m_operationOutput == nullptr)
		{
			return;
		}

		QGuiApplication::clipboard()->setText(m_operationOutput->toPlainText());
		SetStatusMessage("Copied selected run output");
	}

	void LauncherMainWindow::RunSelectedOperation()
	{
		if (m_selectedOperationId.isEmpty())
		{
			if (m_operationOutput != nullptr)
			{
				m_operationOutput->setPlainText("Choose a workflow before running.");
			}
			SetStatusMessage("No workflow selected");
			return;
		}

		if (OperationNeedsProject(m_selectedOperationId) && m_projectModel.SelectedProjectId().isEmpty())
		{
			const QString message = "No project discovered. Run Sync Third Parties or Check Dependencies, then retry.";
			if (m_operationOutput != nullptr)
			{
				m_operationOutput->setPlainText(message);
			}
			SetStatusMessage(message);
			return;
		}

		if ((m_selectedOperationId == "workspace.setup" || m_selectedOperationId == "workspace.generate-solution" || m_selectedOperationId == "workspace.open-solution" ||
		     m_selectedOperationId == "launcher.build.self" || m_selectedOperationId.startsWith("project.build") || m_selectedOperationId == "cook.tools.prepare") &&
		    !OfferWorkspacePrerequisiteOperation(m_selectedOperationId))
		{
			return;
		}

		if (m_selectedOperationId.startsWith("cook.") && m_selectedOperationId != "cook.tools.prepare" && !OfferCookPrerequisiteOperation(m_selectedOperationId))
		{
			return;
		}

		if (FindLaunchOperationDefinition(m_selectedOperationId.toStdString()).has_value() && !OfferLaunchPrerequisiteOperation(m_selectedOperationId))
		{
			return;
		}

		LauncherOperationRequest request = BuildOperationRequest(m_selectedOperationId);
		if (!ConfirmRunRequest(request))
		{
			SetStatusMessage("Run canceled");
			return;
		}

		const QString title = DisplayNameForOperation(m_selectedOperationId);
		StartOperation(std::move(request), title);
	}

	void LauncherMainWindow::DisplayOperationStarted(const QString& runId, const QString&, const QString& title)
	{
		SetRunState(runId, RunState::Running, title);
		AppendRunOutput(runId, title + " started.\n");
		ShowRunOutput(runId);
		SetStatusMessage(title + " running");
		UpdateProgress();
	}

	void LauncherMainWindow::AppendOperationOutput(const QString& runId, const QString&, const QString& outputText)
	{
		AppendRunOutput(runId, outputText);
		if (m_activeRunId == runId)
		{
			ShowRunOutput(runId);
		}
	}

	void LauncherMainWindow::DisplayOperationFinished(const QString& runId, const QString& operationId, const QString& title, const QString& statusText, int exitCode)
	{
		const bool succeeded = exitCode == 0;
		SetRunState(runId, succeeded ? RunState::Done : RunState::Failed, title);

		if (succeeded)
		{
			AppendRunOutput(runId, "\n" + title + " finished: " + statusText + "\n");
		}
		else
		{
			const QString existingOutput = m_runOutputs.value(runId);
			const QString recoveryHint = FailureRecoveryHint(operationId, statusText);
			const QString recoveryText = recoveryHint.isEmpty() ? QString() : QStringLiteral("Recovery: %1\n\n").arg(recoveryHint);
			m_runOutputs.insert(
			    runId,
			    QStringLiteral("Failed: %1 (exit code %2)\n").arg(statusText).arg(exitCode) + recoveryText + "\n" + existingOutput + "\n" + title + " finished: " + statusText + "\n");
			++m_failedRunCount;
		}

		++m_finishedRunCount;
		ShowRunOutput(runId);
		SetStatusMessage(title + " finished: " + statusText);
		UpdateProgress();
		RefreshProjects();
		RebuildOptionsPages();

		if (succeeded && operationId == "launcher.build.self" && !m_pendingRestartRunIds.contains(runId))
		{
			m_pendingRestartRunIds.push_back(runId);
			PromptForLauncherRestart();
		}
	}

	QWidget* LauncherMainWindow::CreateWorkflowSurface()
	{
		QFrame* surface = new QFrame(this);
		surface->setObjectName("WorkflowSurface");
		QHBoxLayout* layout = new QHBoxLayout(surface);
		layout->setContentsMargins(kSpaceMedium, kSpaceMedium, kSpaceMedium, kSpaceMedium);
		layout->setSpacing(kSpaceMedium);
		layout->addWidget(CreateProcessPicker(surface), 0);
		layout->addWidget(CreateOptionsPanel(surface), 1);
		return surface;
	}

	QWidget* LauncherMainWindow::CreateProcessPicker(QWidget* parent)
	{
		QFrame* panel = new QFrame(parent);
		panel->setObjectName("ProcessPanel");
		panel->setFixedWidth(kWorkflowRailWidth);
		QVBoxLayout* layout = new QVBoxLayout(panel);
		layout->setContentsMargins(0, 0, 0, 0);
		layout->setSpacing(kSpaceMedium);

		QLabel* railTitle = new QLabel("Workflows", panel);
		railTitle->setObjectName("WorkflowRailTitle");
		layout->addWidget(railTitle);

		QHBoxLayout* workflowLayout = new QHBoxLayout();
		workflowLayout->setContentsMargins(0, 0, 0, 0);
		workflowLayout->setSpacing(kSpaceMedium);

		QVBoxLayout* groupLayout = new QVBoxLayout();
		groupLayout->setContentsMargins(0, 0, 0, 0);
		groupLayout->setSpacing(kSpaceTiny);

		m_workflowGroupButtonGroup = new QButtonGroup(this);
		m_workflowGroupButtonGroup->setExclusive(true);
		m_processButtonGroup = new QButtonGroup(this);
		m_processButtonGroup->setExclusive(true);

		m_operationStack = new QStackedWidget(panel);
		m_operationStack->setObjectName("OperationStack");

		const QVector<WorkflowDefinition> workflows = CreateWorkflowDefinitions();
		for (int workflowIndex = 0; workflowIndex < workflows.size(); ++workflowIndex)
		{
			const WorkflowDefinition& workflow = workflows[workflowIndex];
			QPushButton* groupButton = new QPushButton(workflow.Title, panel);
			groupButton->setObjectName("WorkflowGroupButton");
			groupButton->setMinimumHeight(kWorkflowGroupMinHeight);
			groupButton->setProperty("WorkflowIndex", workflowIndex);
			groupButton->setProperty("ActiveState", "false");
			groupButton->setAccessibleName(workflow.Title + " workflow group");
			groupButton->setIcon(WorkflowIconForIndex(workflowIndex));
			groupButton->setIconSize(QSize(kLauncherIconSize, kLauncherIconSize));
			RegisterFocusable(groupButton);
			m_workflowGroupButtonGroup->addButton(groupButton);
			groupLayout->addWidget(groupButton);

			QWidget* tabPage = new QWidget(m_operationStack);
			QVBoxLayout* actionLayout = new QVBoxLayout();
			actionLayout->setContentsMargins(0, 0, 0, 0);
			actionLayout->setSpacing(kSpaceTiny);
			for (int index = 0; index < workflow.OperationIds.size(); ++index)
			{
				const QString& operationId = workflow.OperationIds[index];
				QPushButton* button = CreateProcessButton(DisplayNameForOperation(operationId), operationId, tabPage);
				m_processButtonGroup->addButton(button);
				actionLayout->addWidget(button);
			}
			actionLayout->addStretch(1);
			tabPage->setLayout(actionLayout);
			const int pageIndex = m_operationStack->addWidget(tabPage);
			for (const QString& operationId : workflow.OperationIds)
			{
				m_workflowPageByOperation.insert(operationId, pageIndex);
			}
		}
		groupLayout->addStretch(1);
		workflowLayout->addLayout(groupLayout, 0);
		workflowLayout->addWidget(m_operationStack, 1);
		connect(m_workflowGroupButtonGroup, &QButtonGroup::buttonClicked, this, &LauncherMainWindow::SelectWorkflowGroupButton);
		connect(m_processButtonGroup, &QButtonGroup::buttonClicked, this, &LauncherMainWindow::SelectProcessButton);
		layout->addLayout(workflowLayout, 1);
		return panel;
	}

	QPushButton* LauncherMainWindow::CreateProcessButton(const QString& label, const QString& operationId, QWidget* parent)
	{
		QPushButton* button = new QPushButton(label, parent);
		button->setObjectName("WorkflowButton");
		button->setCheckable(true);
		button->setMinimumHeight(kWorkflowButtonMinHeight);
		button->setProperty("OperationId", operationId);
		button->setAccessibleName(label + " workflow");
		RegisterFocusable(button);
		return button;
	}

	QWidget* LauncherMainWindow::CreateOptionsPanel(QWidget* parent)
	{
		QFrame* panel = new QFrame(parent);
		panel->setObjectName("OptionsPanel");
		QVBoxLayout* layout = new QVBoxLayout(panel);
		layout->setContentsMargins(kPanelHorizontalMargin, kPanelVerticalMargin, kPanelHorizontalMargin, kPanelVerticalMargin);
		layout->setSpacing(kSpaceSmall + kSpaceTiny);

		m_activeOperationLabel = new QLabel("No workflow selected", panel);
		m_activeOperationLabel->setObjectName("ActiveOperationLabel");
		m_activeOperationLabel->setAccessibleName("Selected workflow");
		layout->addWidget(m_activeOperationLabel);

		m_optionsStack = new QStackedWidget(panel);
		m_optionsStack->setObjectName("OptionsStack");
		RebuildOptionsPages();
		layout->addWidget(m_optionsStack, 1);
		m_optionsStack->setVisible(false);

		QHBoxLayout* actionLayout = new QHBoxLayout();
		actionLayout->setSpacing(kSpaceSmall + kSpaceTiny);
		actionLayout->addStretch(1);
		m_runButton = new QPushButton("Run", panel);
		m_runButton->setObjectName("PrimaryActionButton");
		m_runButton->setIcon(CreateLauncherIcon(LauncherIcon::Run, QColor("#ffffff")));
		m_runButton->setIconSize(QSize(kLauncherIconSize, kLauncherIconSize));
		m_runButton->setToolTip("Run the selected workflow. Existing runs keep going.");
		m_runButton->setEnabled(false);
		m_runButton->setAccessibleName("Run selected workflow");
		RegisterFocusable(m_runButton);
		connect(m_runButton, &QPushButton::clicked, this, &LauncherMainWindow::RunSelectedOperation);
		actionLayout->addWidget(m_runButton);
		layout->addLayout(actionLayout);
		return panel;
	}

	QWidget* LauncherMainWindow::CreateFooterContextPanel(QWidget* parent)
	{
		QFrame* panel = new QFrame(parent);
		panel->setObjectName("FooterContextPanel");
		QHBoxLayout* rowLayout = new QHBoxLayout(panel);
		rowLayout->setContentsMargins(0, 0, 0, 0);
		rowLayout->setSpacing(kSpaceSmall);
		rowLayout->addStretch(1);

		QLabel* projectLabel = CreateFieldLabel("Project");
		projectLabel->setObjectName("FooterFieldLabel");
		rowLayout->addWidget(projectLabel, 0);
		QComboBox* projectCombo = CreateProjectCombo();
		projectCombo->setObjectName("FooterContextCombo");
		projectCombo->setAccessibleName("Project");
		projectCombo->setToolTip("Global project context used by project, cook, launch, and smoke workflows.");
		projectCombo->setMinimumWidth(180);
		projectCombo->setMaximumWidth(220);
		projectCombo->setMinimumHeight(28);
		projectCombo->setMaximumHeight(28);
		projectLabel->setBuddy(projectCombo);
		rowLayout->addWidget(projectCombo, 0);

		QLabel* configurationLabel = CreateFieldLabel("Build Configuration");
		configurationLabel->setObjectName("FooterFieldLabel");
		rowLayout->addWidget(configurationLabel, 0);
		QComboBox* configurationCombo = CreateValueCombo(
		    {{"Development", "development"}, {"Debug", "debug"}, {"Shipping", "shipping"}},
		    m_settings.BuildConfiguration(),
		    &LauncherSettings::SetBuildConfiguration);
		configurationCombo->setObjectName("FooterContextCombo");
		configurationCombo->setAccessibleName("Build Configuration");
		configurationCombo->setToolTip("Global build configuration used for editor, runtime, and tool workflows.");
		configurationCombo->setMinimumWidth(180);
		configurationCombo->setMaximumWidth(220);
		configurationCombo->setMinimumHeight(28);
		configurationCombo->setMaximumHeight(28);
		configurationLabel->setBuddy(configurationCombo);
		rowLayout->addWidget(configurationCombo, 0);

		QLabel* ideLabel = CreateFieldLabel("IDE");
		ideLabel->setObjectName("FooterFieldLabel");
		rowLayout->addWidget(ideLabel, 0);
		QComboBox* ideCombo = CreateValueCombo({{"Visual Studio", "visual-studio"}, {"Rider", "rider"}}, m_settings.WorkspaceIde(), &LauncherSettings::SetWorkspaceIde);
		ideCombo->setObjectName("FooterContextCombo");
		ideCombo->setAccessibleName("IDE");
		ideCombo->setToolTip("Global IDE choice for workspace generation and opening.");
		ideCombo->setMinimumWidth(150);
		ideCombo->setMaximumWidth(190);
		ideCombo->setMinimumHeight(28);
		ideCombo->setMaximumHeight(28);
		ideLabel->setBuddy(ideCombo);
		rowLayout->addWidget(ideCombo, 0);

		m_footerContextPanel = panel;
		return panel;
	}

	QWidget* LauncherMainWindow::CreateOptionsPage(const QString& operationId, QWidget* parent)
	{
		QScrollArea* scrollArea = new QScrollArea(parent);
		scrollArea->setObjectName("OptionsScrollArea");
		scrollArea->setWidgetResizable(true);
		scrollArea->setFrameShape(QFrame::NoFrame);

		QWidget* content = new QWidget(scrollArea);
		content->setObjectName("OptionsContent");
		QVBoxLayout* layout = new QVBoxLayout(content);
		layout->setContentsMargins(0, kSpaceSmall, kSpaceSmall, 0);
		layout->setSpacing(kSpaceSmall);
		AddOptionsForOperation(*layout, operationId);
		layout->addStretch(1);
		scrollArea->setWidget(content);
		return scrollArea;
	}

	QWidget* LauncherMainWindow::CreateOutputPanel()
	{
		QFrame* panel = new QFrame(this);
		panel->setObjectName("OutputPanel");
		QVBoxLayout* layout = new QVBoxLayout(panel);
		layout->setContentsMargins(kPanelHorizontalMargin, kSpaceMedium + kSpaceTiny, kPanelHorizontalMargin, kSpaceLarge);
		layout->setSpacing(kSpaceSmall + kSpaceTiny);

		QLabel* monitorLabel = CreateSectionLabel("Activity");
		layout->addWidget(monitorLabel);

		QHBoxLayout* progressLayout = new QHBoxLayout();
		progressLayout->setSpacing(kSpaceMedium + kSpaceTiny);
		m_progressLabel = new QLabel("No runs yet", panel);
		m_progressLabel->setObjectName("ProgressLabel");
		progressLayout->addWidget(m_progressLabel);
		m_progressBar = new QProgressBar(panel);
		m_progressBar->setObjectName("ProgressBar");
		m_progressBar->setTextVisible(false);
		progressLayout->addWidget(m_progressBar, 1);
		layout->addLayout(progressLayout);

		QHBoxLayout* activityHeaderLayout = new QHBoxLayout();
		activityHeaderLayout->setSpacing(kSpaceMedium + kSpaceTiny);
		QLabel* activityHeader = CreateFieldLabel("Activity");
		activityHeader->setMinimumWidth(kActivityListWidth);
		activityHeaderLayout->addWidget(activityHeader, 1);
		activityHeaderLayout->addWidget(CreateFieldLabel("Output"), 3);
		m_copyOutputButton = new QPushButton("Copy output", panel);
		m_copyOutputButton->setObjectName("SecondaryButton");
		m_copyOutputButton->setIcon(CreateLauncherIcon(LauncherIcon::Copy, QColor(kColorStateQueued)));
		m_copyOutputButton->setIconSize(QSize(kLauncherIconSize, kLauncherIconSize));
		m_copyOutputButton->setEnabled(false);
		m_copyOutputButton->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_C));
		m_copyOutputButton->setToolTip("Select a run to copy its output. Shortcut: Ctrl+Shift+C.");
		m_copyOutputButton->setAccessibleName("Copy selected run output");
		m_copyOutputButton->setAccessibleDescription("Copies output for the selected run.");
		RegisterFocusable(m_copyOutputButton);
		connect(m_copyOutputButton, &QPushButton::clicked, this, &LauncherMainWindow::CopySelectedRunOutput);
		activityHeaderLayout->addWidget(m_copyOutputButton, 0);

		QHBoxLayout* activityLayout = new QHBoxLayout();
		activityLayout->setSpacing(kSpaceMedium + kSpaceTiny);
		m_activityList = new QListWidget(panel);
		m_activityList->setObjectName("ActivityList");
		m_activityList->setMinimumWidth(kActivityListWidth);
		m_activityList->setMaximumHeight(kActivityListMaxHeight);
		m_activityList->setAccessibleName("Activity runs");
		m_activityList->setAccessibleDescription("Recent runs. Select one to review its summary and output.");
		RegisterFocusable(m_activityList);
		connect(m_activityList, &QListWidget::currentItemChanged, this, &LauncherMainWindow::DisplaySelectedRunOutput);
		activityLayout->addWidget(m_activityList, 1);

		QVBoxLayout* outputLayout = new QVBoxLayout();
		outputLayout->setContentsMargins(0, 0, 0, 0);
		outputLayout->setSpacing(kSpaceSmall);
		m_selectedRunSummary = new QLabel("Select a run to view output.", panel);
		m_selectedRunSummary->setObjectName("ActivitySummary");
		m_selectedRunSummary->setAccessibleName("Selected activity summary");
		m_selectedRunSummary->setWordWrap(true);
		outputLayout->addWidget(m_selectedRunSummary);

		m_operationOutput = new QTextEdit(panel);
		m_operationOutput->setObjectName("OperationOutput");
		m_operationOutput->setReadOnly(true);
		m_operationOutput->setMinimumHeight(kOperationOutputMinHeight);
		m_operationOutput->setMaximumHeight(kOperationOutputMaxHeight);
		m_operationOutput->setToolTip("Select a run to view its output.");
		m_operationOutput->setAccessibleName("Selected run output");
		m_operationOutput->setAccessibleDescription("Read-only output for the selected run.");
		RegisterFocusable(m_operationOutput);
		outputLayout->addWidget(m_operationOutput);
		activityLayout->addLayout(outputLayout, 3);

		m_activityDetailsPanel = new QFrame(panel);
		m_activityDetailsPanel->setObjectName("ActivityDetailsPanel");
		QVBoxLayout* activityDetailsLayout = new QVBoxLayout(m_activityDetailsPanel);
		activityDetailsLayout->setContentsMargins(0, 0, 0, 0);
		activityDetailsLayout->setSpacing(kSpaceSmall);
		activityDetailsLayout->addLayout(activityHeaderLayout);
		activityDetailsLayout->addLayout(activityLayout);
		layout->addWidget(m_activityDetailsPanel);
		return panel;
	}

	QLabel* LauncherMainWindow::CreateSectionLabel(const QString& title) const
	{
		QLabel* label = new QLabel(title);
		label->setObjectName("SectionLabel");
		label->setAccessibleName(title);
		return label;
	}

	QLabel* LauncherMainWindow::CreateFieldLabel(const QString& title) const
	{
		QLabel* label = new QLabel(title);
		label->setObjectName("FieldLabel");
		label->setAccessibleName(title);
		return label;
	}

	QCheckBox* LauncherMainWindow::CreateBoundCheckBox(const QString& label, const QString& tooltip, bool checked, void (LauncherSettings::*setter)(bool))
	{
		QCheckBox* box = new QCheckBox(label, this);
		box->setToolTip(tooltip);
		box->setAccessibleName(label);
		box->setAccessibleDescription(tooltip);
		box->setChecked(checked);
		RegisterFocusable(box);
		connect(box, &QCheckBox::toggled, &m_settings, setter);
		return box;
	}

	QLineEdit* LauncherMainWindow::CreateBoundLineEdit(const QString& text, const QString& placeholder, const QString& tooltip, void (LauncherSettings::*setter)(const QString&))
	{
		QLineEdit* edit = new QLineEdit(this);
		edit->setText(text);
		edit->setPlaceholderText(placeholder);
		edit->setToolTip(tooltip);
		edit->setAccessibleDescription(tooltip);
		RegisterFocusable(edit);
		connect(edit, &QLineEdit::textChanged, &m_settings, setter);
		return edit;
	}

	QTextEdit* LauncherMainWindow::CreateBoundTextEdit(const QString& text, const QString& placeholder, const QString& tooltip, void (LauncherSettings::*setter)(const QString&))
	{
		QTextEdit* edit = new QTextEdit(this);
		edit->setPlainText(text);
		edit->setPlaceholderText(placeholder);
		edit->setToolTip(tooltip);
		edit->setAccessibleDescription(tooltip);
		edit->setMinimumHeight(78);
		edit->setMaximumHeight(118);
		RegisterFocusable(edit);
		connect(edit, &QTextEdit::textChanged, this, [edit, setter, this]() {
			(m_settings.*setter)(edit->toPlainText());
		});
		return edit;
	}

	QComboBox* LauncherMainWindow::CreateProfileCombo(const QStringList& profiles, const QString& currentProfile, void (LauncherSettings::*setter)(const QString&))
	{
		QComboBox* combo = new QComboBox(this);
		combo->addItems(profiles);
		combo->setAccessibleName("Profile");
		combo->setAccessibleDescription("Build profile used by this workflow.");
		combo->setCurrentText(currentProfile);
		RegisterFocusable(combo);
		connect(combo, &QComboBox::currentTextChanged, &m_settings, setter);
		return combo;
	}

	QComboBox* LauncherMainWindow::CreateProjectCombo()
	{
		QComboBox* combo = new QComboBox(this);
		combo->setObjectName("ProjectCombo");
		combo->setProperty("ProjectSelector", true);
		combo->setToolTip("Project used by this workflow.");
		combo->setAccessibleName("Project");
		combo->setAccessibleDescription("Project used by this workflow.");
		RegisterFocusable(combo);
		m_projectSelectors.push_back(combo);
		connect(combo, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), [combo, this]() {
			const QString projectId = combo->currentData().toString();
			if (!projectId.isEmpty())
			{
				m_projectModel.SelectProject(projectId);
				SetStatusMessage("Selected project: " + combo->currentText());
			}
		});
		PopulateProjectCombo(*combo);
		return combo;
	}

	QComboBox* LauncherMainWindow::CreateValueCombo(const QVector<QPair<QString, QString>>& options, const QString& currentValue, void (LauncherSettings::*setter)(const QString&))
	{
		QComboBox* combo = new QComboBox(this);
		combo->setAccessibleName("Option value");
		RegisterFocusable(combo);
		for (const QPair<QString, QString>& option : options)
		{
			combo->addItem(option.first, option.second);
		}
		const int currentIndex = combo->findData(currentValue);
		combo->setCurrentIndex(currentIndex >= 0 ? currentIndex : 0);
		connect(combo, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), [combo, setter, this]() {
			(m_settings.*setter)(combo->currentData().toString());
		});
		return combo;
	}

	void LauncherMainWindow::AddOptionsForOperation(QVBoxLayout& layout, const QString& operationId)
	{
		if (operationId == "workspace.generate-solution" || operationId == "workspace.open-solution" || operationId == "toolchain.check" || operationId == "workspace.setup")
		{
			AddBuildEnvironmentStatus(layout, operationId);
			return;
		}

		if (operationId == "project.build.editor")
		{
			AddBuildEnvironmentStatus(layout, operationId);
			return;
		}

		if (operationId == "launcher.build.self")
		{
			AddBuildEnvironmentStatus(layout, operationId);
			return;
		}

		if (operationId == "project.build.runtime")
		{
			AddBuildEnvironmentStatus(layout, operationId);
			return;
		}

		if (operationId == "cook.tools.prepare")
		{
			AddBuildEnvironmentStatus(layout, operationId);
			return;
		}

		if (operationId == "cook.shaders")
		{
			AddOptionField(layout, "Shader package", CreateValueCombo(
			    {{"All shader packages", ""},
			     {"ComputeClear", "ComputeClear"},
			     {"DirectLighting", "DirectLighting"},
			     {"GBuffer", "GBuffer"},
			     {"HelloInlineRayQuery", "HelloInlineRayQuery"},
			     {"HelloRayTracingLibrary", "HelloRayTracingLibrary"},
			     {"HelloTriangle", "HelloTriangle"},
			     {"IndirectLighting", "IndirectLighting"},
			     {"LightingComposite", "LightingComposite"},
			     {"Sky", "Sky"},
			     {"VisualizeBuffers", "VisualizeBuffers"}},
			    m_settings.ShaderPackages(),
			    &LauncherSettings::SetShaderPackages));
			QCheckBox* forceRecookBox = CreateBoundCheckBox("Clean before cooking", "Remove cooked outputs before this cook.", m_settings.ForceRecook(), &LauncherSettings::SetForceRecook);
			forceRecookBox->setObjectName("WarningCheckBox");
			AddOptionCheckBox(layout, forceRecookBox);
			QCheckBox* confirmRecookBox = CreateBoundCheckBox("Confirm clean cook", "Required before removing cooked outputs.", m_settings.ConfirmForceRecook(), &LauncherSettings::SetConfirmForceRecook);
			confirmRecookBox->setObjectName("DestructiveCheckBox");
			QWidget* confirmRecookRow = AddOptionCheckBox(layout, confirmRecookBox);
			confirmRecookRow->setVisible(forceRecookBox->isChecked());
			connect(forceRecookBox, &QCheckBox::toggled, confirmRecookRow, [confirmRecookRow, confirmRecookBox](bool enabled) {
				confirmRecookRow->setVisible(enabled);
				if (!enabled)
				{
					confirmRecookBox->setChecked(false);
				}
			});
			return;
		}

		if (operationId.startsWith("cook."))
		{
			QCheckBox* forceRecookBox = CreateBoundCheckBox("Clean before cooking", "Remove cooked outputs before this cook.", m_settings.ForceRecook(), &LauncherSettings::SetForceRecook);
			forceRecookBox->setObjectName("WarningCheckBox");
			AddOptionCheckBox(layout, forceRecookBox);
			QCheckBox* confirmRecookBox = CreateBoundCheckBox("Confirm clean cook", "Required before removing cooked outputs.", m_settings.ConfirmForceRecook(), &LauncherSettings::SetConfirmForceRecook);
			confirmRecookBox->setObjectName("DestructiveCheckBox");
			QWidget* confirmRecookRow = AddOptionCheckBox(layout, confirmRecookBox);
			confirmRecookRow->setVisible(forceRecookBox->isChecked());
			connect(forceRecookBox, &QCheckBox::toggled, confirmRecookRow, [confirmRecookRow, confirmRecookBox](bool enabled) {
				confirmRecookRow->setVisible(enabled);
				if (!enabled)
				{
					confirmRecookBox->setChecked(false);
				}
			});
			return;
		}

		if (operationId == "project.open.editor" || operationId == "project.open.runtime")
		{
			QVBoxLayout* appOptionsLayout = AddOptionGroup(layout, "Application Options", "Arguments and runtime CVars passed to the selected process.");
			AddOptionField(*appOptionsLayout, "Graphics backend", CreateValueCombo({{"D3D12", ""}, {"Vulkan", "vulkan"}}, m_settings.LaunchBackend(), &LauncherSettings::SetLaunchBackend));
			AddOptionField(*appOptionsLayout, "VSync", CreateValueCombo({{"On", ""}, {"Off", "false"}}, m_settings.LaunchVSync(), &LauncherSettings::SetLaunchVSync));
			AddOptionField(*appOptionsLayout, "GPU preference", CreateValueCombo({{"High performance", ""}, {"System default", "false"}}, m_settings.LaunchHighPerformanceAdapter(), &LauncherSettings::SetLaunchHighPerformanceAdapter));
			AddOptionField(*appOptionsLayout, "Mesh batching", CreateValueCombo({{"On", ""}, {"Off", "false"}}, m_settings.LaunchMeshAutoBatching(), &LauncherSettings::SetLaunchMeshAutoBatching));
			AddOptionField(*appOptionsLayout, "Arguments", CreateBoundLineEdit(m_settings.LaunchCommandLineArguments(), "--flag value \"quoted value\"", "Extra command-line arguments appended after launcher-managed options.", &LauncherSettings::SetLaunchCommandLineArguments));
			AddOptionField(*appOptionsLayout, "CVars", CreateBoundTextEdit(m_settings.LaunchCVars(), "r.SomeCVar=1\nr.OtherCVar=false", "One CVar assignment per line, comma, or semicolon. Each entry is passed as --cvar name=value.", &LauncherSettings::SetLaunchCVars));
			return;
		}

		if (operationId == "project.run.smoke")
		{
			QVBoxLayout* modeLayout = AddOptionGroup(layout, "Smoke Target", "Choose which project executable should run with smoke validation.");
			AddOptionField(*modeLayout, "Target", CreateValueCombo({{"Editor", "editor"}, {"Runtime", "runtime"}}, m_settings.LaunchTarget(), &LauncherSettings::SetLaunchTarget));

			QVBoxLayout* appOptionsLayout = AddOptionGroup(layout, "Application Options", "Arguments and runtime CVars passed to the selected process.");
			AddOptionField(*appOptionsLayout, "Graphics backend", CreateValueCombo({{"D3D12", ""}, {"Vulkan", "vulkan"}}, m_settings.LaunchBackend(), &LauncherSettings::SetLaunchBackend));
			AddOptionField(*appOptionsLayout, "VSync", CreateValueCombo({{"On", ""}, {"Off", "false"}}, m_settings.LaunchVSync(), &LauncherSettings::SetLaunchVSync));
			AddOptionField(*appOptionsLayout, "GPU preference", CreateValueCombo({{"High performance", ""}, {"System default", "false"}}, m_settings.LaunchHighPerformanceAdapter(), &LauncherSettings::SetLaunchHighPerformanceAdapter));
			AddOptionField(*appOptionsLayout, "Mesh batching", CreateValueCombo({{"On", ""}, {"Off", "false"}}, m_settings.LaunchMeshAutoBatching(), &LauncherSettings::SetLaunchMeshAutoBatching));
			AddOptionField(*appOptionsLayout, "Arguments", CreateBoundLineEdit(m_settings.LaunchCommandLineArguments(), "--flag value \"quoted value\"", "Extra command-line arguments appended after launcher-managed options.", &LauncherSettings::SetLaunchCommandLineArguments));
			AddOptionField(*appOptionsLayout, "CVars", CreateBoundTextEdit(m_settings.LaunchCVars(), "r.SomeCVar=1\nr.OtherCVar=false", "One CVar assignment per line, comma, or semicolon. Each entry is passed as --cvar name=value.", &LauncherSettings::SetLaunchCVars));
			QVBoxLayout* smokeOptionsLayout = AddOptionGroup(layout, "Validation Options", "Smoke-test controls for capture length and diagnostic behavior.");
			AddOptionField(*smokeOptionsLayout, "Frame limit", CreateValueCombo({{"120 frames", ""}, {"60 frames", "60"}, {"300 frames", "300"}, {"600 frames", "600"}}, m_settings.SmokeFrameLimit(), &LauncherSettings::SetSmokeFrameLimit));
			AddOptionField(*smokeOptionsLayout, "Smoke backend", CreateValueCombo({{"D3D12", ""}, {"Vulkan", "vulkan"}}, m_settings.SmokeBackend(), &LauncherSettings::SetSmokeBackend));
			AddOptionCheckBox(*smokeOptionsLayout, CreateBoundCheckBox("Capture trace", "Write smoke trace output.", m_settings.SmokeTrace(), &LauncherSettings::SetSmokeTrace));
			AddOptionCheckBox(*smokeOptionsLayout, CreateBoundCheckBox("Skip level switching", "Do not switch levels during smoke.", m_settings.SmokeSkipLevelSwitching(), &LauncherSettings::SetSmokeSkipLevelSwitching));
			return;
		}

		if (operationId == "project.run")
		{
			QVBoxLayout* modeLayout = AddOptionGroup(layout, "Run Mode", "Choose which project executable to run and whether smoke validation should be enabled.");
			AddOptionField(*modeLayout, "Target", CreateValueCombo({{"Editor", "editor"}, {"Runtime", "runtime"}}, m_settings.LaunchTarget(), &LauncherSettings::SetLaunchTarget));
			QCheckBox* smokeTestBox = CreateBoundCheckBox("Enable smoke test", "Run this launch with smoke validation enabled.", m_settings.LaunchSmokeTest(), &LauncherSettings::SetLaunchSmokeTest);
			AddOptionCheckBox(*modeLayout, smokeTestBox);

			QVBoxLayout* appOptionsLayout = AddOptionGroup(layout, "Application Options", "Arguments and runtime CVars passed to the selected process.");
			AddOptionField(*appOptionsLayout, "Graphics backend", CreateValueCombo({{"D3D12", ""}, {"Vulkan", "vulkan"}}, m_settings.LaunchBackend(), &LauncherSettings::SetLaunchBackend));
			AddOptionField(*appOptionsLayout, "VSync", CreateValueCombo({{"On", ""}, {"Off", "false"}}, m_settings.LaunchVSync(), &LauncherSettings::SetLaunchVSync));
			AddOptionField(*appOptionsLayout, "GPU preference", CreateValueCombo({{"High performance", ""}, {"System default", "false"}}, m_settings.LaunchHighPerformanceAdapter(), &LauncherSettings::SetLaunchHighPerformanceAdapter));
			AddOptionField(*appOptionsLayout, "Mesh batching", CreateValueCombo({{"On", ""}, {"Off", "false"}}, m_settings.LaunchMeshAutoBatching(), &LauncherSettings::SetLaunchMeshAutoBatching));
			AddOptionField(*appOptionsLayout, "Arguments", CreateBoundLineEdit(m_settings.LaunchCommandLineArguments(), "--flag value \"quoted value\"", "Extra command-line arguments appended after launcher-managed options.", &LauncherSettings::SetLaunchCommandLineArguments));
			AddOptionField(*appOptionsLayout, "CVars", CreateBoundTextEdit(m_settings.LaunchCVars(), "r.SomeCVar=1\nr.OtherCVar=false", "One CVar assignment per line, comma, or semicolon. Each entry is passed as --cvar name=value.", &LauncherSettings::SetLaunchCVars));
			QVBoxLayout* smokeOptionsLayout = AddOptionGroup(layout, "Validation Options", "Smoke-test controls for capture length and diagnostic behavior.");
			QWidget* smokeOptionsPanel = smokeOptionsLayout->parentWidget();
			AddOptionField(*smokeOptionsLayout, "Frame limit", CreateValueCombo({{"120 frames", ""}, {"60 frames", "60"}, {"300 frames", "300"}, {"600 frames", "600"}}, m_settings.SmokeFrameLimit(), &LauncherSettings::SetSmokeFrameLimit));
			AddOptionField(*smokeOptionsLayout, "Smoke backend", CreateValueCombo({{"D3D12", ""}, {"Vulkan", "vulkan"}}, m_settings.SmokeBackend(), &LauncherSettings::SetSmokeBackend));
			AddOptionCheckBox(*smokeOptionsLayout, CreateBoundCheckBox("Capture trace", "Write smoke trace output.", m_settings.SmokeTrace(), &LauncherSettings::SetSmokeTrace));
			AddOptionCheckBox(*smokeOptionsLayout, CreateBoundCheckBox("Skip level switching", "Do not switch levels during smoke.", m_settings.SmokeSkipLevelSwitching(), &LauncherSettings::SetSmokeSkipLevelSwitching));
			smokeOptionsPanel->setVisible(m_settings.LaunchSmokeTest());
			connect(smokeTestBox, &QCheckBox::toggled, smokeOptionsPanel, &QWidget::setVisible);
			return;
		}

		if (operationId == "quality.format")
		{
			AddNoOptionsMessage(layout, "Runs clang-format on engine and project source files.");
			return;
		}

		if (operationId == "workspace.clean")
		{
			const std::array<CleanScopeUiOption, 7> cleanScopes = {{
			    {"Project Cooked Data", "selected-cooked", "Cooked asset outputs for the selected project.", QString(), "Cooked Outputs"},
			    {"All Cooked Data", "all-cooked", "Cooked asset outputs for every project.", QString(), "Cooked Outputs"},
			    {"Build Artifacts", "build-tree", "Build outputs, intermediates, generated CMake/Visual Studio files, and local IDE state. Keeps the third-party cache.", "build contents except build/_deps, .vs, root generated project files, project generated files", "Build and Generated State"},
			    {"Shader Cache", "shader-cache", "Transient shader cache, recook signal, debug artifacts, and shader outputs.", QString(), "Caches"},
			    {"Third-Party Cache", "deps", "Downloaded third-party dependency cache. Configure will re-download dependencies.", QString(), "Caches"},
			    {"Log Files", "logs", "Repository, launcher, and project logs.", "logs, build/Launcher/Logs, Projects/*/logs", "Logs"},
			    {"Generated Workspace", "pristine", "All generated workspace state, including the third-party cache, cooked data, IDE state, logs, and generated project files.", "build, .vs, .vscode, logs, imgui.ini, root generated project files, project generated files", "Reset Everything"},
			}};

			QVector<QCheckBox*> scopeBoxes;
			const QString selectedProjectId = m_projectModel.SelectedProjectId();
			const QStringList selectedScopes = m_settings.CleanScope().split(QRegularExpression("[,;\\n]"), Qt::SkipEmptyParts);
			const std::array<QPair<QString, QString>, 5> cleanGroups = {{
			    {"Cooked Outputs", "Remove cooked assets for one project or every project."},
			    {"Build and Generated State", "Remove generated build products and local IDE workspace state."},
			    {"Caches", "Remove caches that will be recreated by later workflows."},
			    {"Logs", "Remove repository, launcher, and project logs."},
			    {"Reset Everything", "Clear nearly all generated workspace state in one pass."},
			}};

			const auto addCleanScopeRow = [this, &scopeBoxes, &selectedProjectId, &selectedScopes, &layout](QVBoxLayout& groupLayout, const CleanScopeUiOption& scope) {
				QCheckBox* scopeBox = new QCheckBox(scope.Label, this);
				scopeBox->setToolTip(scope.Detail);
				scopeBox->setProperty("CleanScope", scope.Value);
				scopeBox->setChecked(selectedScopes.contains(scope.Value) || (selectedScopes.empty() && scope.Value == "selected-cooked"));
				RegisterFocusable(scopeBox);

				QFrame* scopeRow = new QFrame(this);
				scopeRow->setObjectName("OptionCheckRow");
				QVBoxLayout* scopeRowLayout = new QVBoxLayout(scopeRow);
				scopeRowLayout->setContentsMargins(0, 0, 0, 0);
				scopeRowLayout->setSpacing(kSpaceTiny);
				scopeRowLayout->addWidget(scopeBox);
				const std::filesystem::path previewPath = ResolveCleanScopePreviewPath(m_repositoryRoot, selectedProjectId, scope.Value);
				const QString previewText = scope.Preview.isEmpty() ? ToDisplayPath(m_repositoryRoot, previewPath) + " - " + FormatDirectoryInventory(previewPath) : scope.Preview;
				QLabel* scopeDetail = new QLabel(previewText, scopeRow);
				scopeDetail->setObjectName("OptionHelpText");
				scopeDetail->setWordWrap(true);
				scopeRowLayout->addWidget(scopeDetail);
				groupLayout.addWidget(scopeRow);
				scopeBoxes.push_back(scopeBox);
			};

			for (const QPair<QString, QString>& cleanGroup : cleanGroups)
			{
				QVBoxLayout* cleanGroupLayout = AddOptionGroup(layout, cleanGroup.first, cleanGroup.second);
				for (const CleanScopeUiOption& scope : cleanScopes)
				{
					if (scope.Group == cleanGroup.first)
					{
						addCleanScopeRow(*cleanGroupLayout, scope);
					}
				}
			}

			const auto updateCleanScopeSetting = [scopeBoxes, this]() {
				QStringList selectedValues;
				for (QCheckBox* scopeBox : scopeBoxes)
				{
					if (scopeBox != nullptr && scopeBox->isChecked())
					{
						selectedValues.push_back(scopeBox->property("CleanScope").toString());
					}
				}
				if (selectedValues.empty())
				{
					selectedValues.push_back("selected-cooked");
				}
				m_settings.SetCleanScope(selectedValues.join(';'));
				UpdateRunAvailability();
			};
			for (QCheckBox* scopeBox : scopeBoxes)
			{
				connect(scopeBox, &QCheckBox::toggled, this, updateCleanScopeSetting);
			}
			updateCleanScopeSetting();
			return;
		}

		AddNoOptionsMessage(layout, "No settings");
	}

	QWidget* LauncherMainWindow::AddOptionField(QVBoxLayout& layout, const QString& label, QWidget* control)
	{
		QFrame* row = new QFrame(this);
		row->setObjectName("OptionRow");
		QHBoxLayout* rowLayout = new QHBoxLayout(row);
		rowLayout->setContentsMargins(0, 0, 0, 0);
		rowLayout->setSpacing(kSpaceMedium + kSpaceTiny);

		QLabel* fieldLabel = CreateFieldLabel(label);
		fieldLabel->setAlignment(Qt::AlignLeft | (qobject_cast<QTextEdit*>(control) != nullptr ? Qt::AlignTop : Qt::AlignVCenter));
		fieldLabel->setFixedWidth(kFieldLabelWidth);
		fieldLabel->setBuddy(control);
		if (control->accessibleName().isEmpty() || control->accessibleName() == "Option value")
		{
			control->setAccessibleName(label);
		}
		if (control->toolTip().isEmpty())
		{
			control->setToolTip("Choose " + label.toLower() + " for this workflow.");
		}
		rowLayout->addWidget(fieldLabel);
		rowLayout->addWidget(control, 1);
		layout.addWidget(row);
		return row;
	}

	QWidget* LauncherMainWindow::AddOptionCheckBox(QVBoxLayout& layout, QCheckBox* checkBox)
	{
		QFrame* row = new QFrame(this);
		row->setObjectName("OptionRow");
		QHBoxLayout* rowLayout = new QHBoxLayout(row);
		rowLayout->setContentsMargins(0, 0, 0, 0);
		rowLayout->setSpacing(kSpaceMedium + kSpaceTiny);
		rowLayout->addSpacing(kFieldLabelWidth + kSpaceMedium + kSpaceTiny);
		rowLayout->addWidget(checkBox, 1);
		layout.addWidget(row);
		return row;
	}

	QVBoxLayout* LauncherMainWindow::AddOptionGroup(QVBoxLayout& layout, const QString& title, const QString& detail)
	{
		QFrame* group = new QFrame(this);
		group->setObjectName("OptionGroup");
		QVBoxLayout* groupLayout = new QVBoxLayout(group);
		groupLayout->setContentsMargins(kSpaceMedium, kSpaceSmall + kSpaceTiny, kSpaceMedium, kSpaceMedium);
		groupLayout->setSpacing(kSpaceSmall);

		QLabel* titleLabel = new QLabel(title, group);
		titleLabel->setObjectName("OptionGroupTitle");
		groupLayout->addWidget(titleLabel);

		if (!detail.isEmpty())
		{
			QLabel* detailLabel = new QLabel(detail, group);
			detailLabel->setObjectName("OptionHelpText");
			detailLabel->setWordWrap(true);
			groupLayout->addWidget(detailLabel);
		}

		layout.addWidget(group);
		return groupLayout;
	}

	void LauncherMainWindow::AddStatusRow(QVBoxLayout& layout, const QString& label, const QString& status, const QString& detail, const QString& state)
	{
		QFrame* row = new QFrame(this);
		row->setObjectName("StatusRow");
		QVBoxLayout* rowLayout = new QVBoxLayout(row);
		rowLayout->setContentsMargins(0, 0, 0, 0);
		rowLayout->setSpacing(kSpaceTiny);

		QHBoxLayout* summaryLayout = new QHBoxLayout();
		summaryLayout->setContentsMargins(0, 0, 0, 0);
		summaryLayout->setSpacing(kSpaceSmall);

		QLabel* nameLabel = new QLabel(label, row);
		nameLabel->setObjectName("StatusLabel");
		summaryLayout->addWidget(nameLabel, 1);

		QLabel* statusLabel = new QLabel(status, row);
		statusLabel->setObjectName("StatusValue");
		statusLabel->setProperty("State", state);
		summaryLayout->addWidget(statusLabel, 0, Qt::AlignRight);
		rowLayout->addLayout(summaryLayout);

		if (!detail.isEmpty())
		{
			QLabel* detailLabel = new QLabel(detail, row);
			detailLabel->setObjectName("StatusDetail");
			detailLabel->setWordWrap(true);
			rowLayout->addWidget(detailLabel);
		}

		layout.addWidget(row);
	}

	void LauncherMainWindow::AddBuildEnvironmentStatus(QVBoxLayout& layout, const QString& operationId)
	{
		BuildWorkspaceOperationRequest request;
		request.RepositoryRoot = m_repositoryRoot;
		request.ProjectId = m_projectModel.SelectedProjectId().isEmpty() ? std::string("Showcase") : m_projectModel.SelectedProjectId().toStdString();
		request.EditorProfile = m_settings.EditorProfile().toStdString();
		request.RuntimeProfile = m_settings.RuntimeProfile().toStdString();
		request.PreferredIde = SelectedWorkspaceIde(m_settings);
		request.ForceConfigure = m_settings.ForceConfigure();

		const BuildWorkspaceOperationPlan plan = PlanBuildWorkspaceOperation(operationId.toStdString(), request);
		const QString workspaceIdeName = SelectedWorkspaceIdeName(m_settings);
		const bool isToolchainCheck = operationId == "toolchain.check";
		const bool isSetupWorkflow = operationId == "workspace.setup" || operationId == "workspace.generate-solution" || operationId == "workspace.open-solution";
		const bool isBuildWorkflow = operationId.startsWith("project.build") || operationId == "cook.tools.prepare" || operationId == "launcher.build.self";
		const std::filesystem::path dependencyCachePath = GetBuildDirectory(m_repositoryRoot) / "_deps";
		const bool dependencyCacheReady = DirectoryHasEntries(dependencyCachePath);

		if (isToolchainCheck)
		{
			QVBoxLayout* toolchainLayout = AddOptionGroup(layout, "Required Dependencies", "Only the machine dependencies needed before build, cook, and IDE workflows can run.");
			AddStatusRow(*toolchainLayout, "Dependency set", plan.Toolchain.RequiredToolsAvailable ? "Ready" : "Action needed", BuildGeneratorSummary(plan.Toolchain), plan.Toolchain.RequiredToolsAvailable ? "ok" : "bad");
			for (const ToolchainItemStatus& item : plan.Toolchain.Items)
			{
				QString detail = QString::fromStdString(item.Detail);
				const QString path = FormatStatusPath(item.Path);
				if (!path.isEmpty())
				{
					detail = CombineStatusDetail(detail, path);
				}
				AddStatusRow(
				    *toolchainLayout,
				    QString::fromStdString(item.DisplayName) + (item.Required ? "" : " (optional)"),
				    ToolchainStatusText(item.State, item.Required),
				    detail,
				    ToolchainStatusState(item.State, item.Required));
			}
			AddStatusRow(
			    *toolchainLayout,
			    "Selected IDE",
			    workspaceIdeName,
			    request.PreferredIde == WorkspaceIde::Rider ? (plan.Toolchain.RiderPath.empty() ? "Rider executable was not found." : QString::fromStdString(plan.Toolchain.RiderPath.string())) :
			                                                 (plan.Toolchain.VswherePath.empty() ? "Visual Studio discovery is not ready." : QString::fromStdString(plan.Freshness.SolutionPath.string())),
			    request.PreferredIde == WorkspaceIde::Rider ? (plan.Toolchain.RiderPath.empty() ? "warning" : "ok") : (plan.Toolchain.VswherePath.empty() ? "warning" : "ok"));

			return;
		}

		if (isSetupWorkflow)
		{
			const QString buildFilesLabel = operationId == "workspace.generate-solution" ? "Generated solution files" :
			                               operationId == "workspace.open-solution" ? "IDE workspace files" :
			                                                                        "Configured workspace files";
			const QString buildFilesDetail = CombineStatusDetail(
			    QString::fromStdString(plan.Freshness.Summary),
			    request.PreferredIde == WorkspaceIde::Rider ? QString::fromStdString(m_repositoryRoot.string()) : QString::fromStdString(plan.Freshness.SolutionPath.string()));
			const QString cacheStatus = dependencyCacheReady ? "Ready" : "Will be created";
			const QString cacheDetail = dependencyCacheReady ?
			                               QString("Third-party cache available at %1.").arg(QString::fromStdString(dependencyCachePath.string())) :
			                               QString("Third-party cache will be populated under %1 when Sync Third Parties runs.").arg(QString::fromStdString(dependencyCachePath.string()));
			QVBoxLayout* workspaceLayout = AddOptionGroup(layout, "Action Dependencies", "State this setup workflow depends on before it can do useful work.");
			AddStatusRow(*workspaceLayout, buildFilesLabel, plan.Freshness.Current ? "Ready" : "Needs refresh", buildFilesDetail, plan.Freshness.Current ? "ok" : "warning");
			AddStatusRow(*workspaceLayout, "Third-Party Cache", cacheStatus, cacheDetail, dependencyCacheReady ? "ok" : "warning");
			AddStatusRow(
			    *workspaceLayout,
			    "IDE output target",
			    workspaceIdeName,
			    request.PreferredIde == WorkspaceIde::Rider ? "Rider opens the repository root." : QString("Visual Studio opens %1.").arg(QString::fromStdString(plan.Freshness.SolutionPath.string())),
			    "neutral");

			if (!plan.Toolchain.RequiredToolsAvailable)
			{
				AddStatusRow(*workspaceLayout, "Required tools", "Blocked", RequiredToolProblemSummary(plan.Toolchain), "bad");
			}
			return;
		}

		if (isBuildWorkflow)
		{
			QVBoxLayout* buildLayout = AddOptionGroup(layout, "Action Dependencies", "State this build workflow depends on before it can do useful work.");
			AddStatusRow(*buildLayout, "Required tools", plan.Toolchain.RequiredToolsAvailable ? "Ready" : "Blocked", plan.Toolchain.RequiredToolsAvailable ? BuildGeneratorSummary(plan.Toolchain) : RequiredToolProblemSummary(plan.Toolchain), plan.Toolchain.RequiredToolsAvailable ? "ok" : "bad");
			AddStatusRow(*buildLayout, "Build files", plan.Freshness.Current ? "Ready" : "Needs refresh", QString::fromStdString(plan.Freshness.Summary), plan.Freshness.Current ? "ok" : "warning");
			const QString targetDetail =
			    operationId == "launcher.build.self" ? QString("SparkleLauncher for ") + m_settings.BuildConfiguration() :
			                                           (plan.PlannedEffects.empty() ? QString("No target resolved for the selected project/profile.") : QString::fromStdString(plan.PlannedEffects.back()));
			AddStatusRow(*buildLayout, "Target", plan.CanRun ? "Resolved" : "Blocked", targetDetail, plan.CanRun ? "ok" : "warning");
		}
	}

	QVBoxLayout* LauncherMainWindow::AddInlineOptionsSection(QVBoxLayout& layout)
	{
		QFrame* section = new QFrame(this);
		section->setObjectName("InlineOptionsSection");
		QVBoxLayout* sectionLayout = new QVBoxLayout(section);
		sectionLayout->setContentsMargins(0, 0, 0, 0);
		sectionLayout->setSpacing(kSpaceSmall);
		layout.addWidget(section);
		return sectionLayout;
	}

	void LauncherMainWindow::AddNoOptionsMessage(QVBoxLayout& layout, const QString& text)
	{
		QLabel* label = new QLabel(text, this);
		label->setObjectName("MutedLabel");
		label->setAccessibleName(text);
		label->setWordWrap(true);
		layout.addWidget(label);
	}

	void LauncherMainWindow::SetControlsEnabled(bool enabled)
	{
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
		if (m_optionsStack == nullptr)
		{
			return;
		}

		while (m_optionsStack->count() > 0)
		{
			QWidget* page = m_optionsStack->widget(0);
			m_optionsStack->removeWidget(page);
			page->deleteLater();
		}
		m_optionsPageByOperation.clear();

		for (const WorkflowDefinition& workflow : CreateWorkflowDefinitions())
		{
			for (const QString& operationId : workflow.OperationIds)
			{
				if (m_optionsPageByOperation.contains(operationId))
				{
					continue;
				}

				const int pageIndex = m_optionsStack->addWidget(CreateOptionsPage(operationId, m_optionsStack));
				m_optionsPageByOperation.insert(operationId, pageIndex);
			}
		}

		if (!m_selectedOperationId.isEmpty() && m_optionsPageByOperation.contains(m_selectedOperationId))
		{
			m_optionsStack->setCurrentIndex(m_optionsPageByOperation.value(m_selectedOperationId));
		}

		m_projectSelectors.clear();
		for (QComboBox* combo : findChildren<QComboBox*>())
		{
			if (combo != nullptr && combo->property("ProjectSelector").toBool())
			{
				m_projectSelectors.push_back(combo);
			}
		}
	}

	void LauncherMainWindow::LoadLauncherIconFont()
	{
#ifdef SPARKLE_FONT_AWESOME_SOLID_TTF
		const char* fontPath = SPARKLE_FONT_AWESOME_SOLID_TTF;
		std::error_code errorCode;
		if (!std::filesystem::exists(fontPath, errorCode) || errorCode)
		{
			return;
		}

		const int fontId = QFontDatabase::addApplicationFont(QString::fromUtf8(fontPath));
		if (fontId < 0)
		{
			return;
		}

		const QStringList families = QFontDatabase::applicationFontFamilies(fontId);
		if (!families.empty())
		{
			m_iconFontFamily = families.front();
		}
#endif
	}

	QIcon LauncherMainWindow::CreateApplicationIcon() const
	{
		QIcon icon;
		for (const int size : {16, 24, 32, 48, 64})
		{
			QPixmap pixmap(size, size);
			pixmap.fill(Qt::transparent);

			QPainter painter(&pixmap);
			painter.setRenderHint(QPainter::Antialiasing, true);
			const QRectF bounds(1.0, 1.0, size - 2.0, size - 2.0);
			const qreal radius = qMax(3.0, size * 0.18);
			painter.setPen(QColor("#3d4652"));
			painter.setBrush(QColor("#1f242b"));
			painter.drawRoundedRect(bounds, radius, radius);

			painter.setPen(Qt::NoPen);
			painter.setBrush(QColor("#0969da"));
			painter.drawRoundedRect(QRectF(size * 0.18, size * 0.18, size * 0.64, size * 0.16), radius * 0.45, radius * 0.45);
			painter.setBrush(QColor("#7ee787"));
			painter.drawEllipse(QRectF(size * 0.62, size * 0.62, size * 0.18, size * 0.18));

			QFont font("Segoe UI");
			font.setBold(true);
			font.setPixelSize(qMax(10, static_cast<int>(size * 0.48)));
			painter.setFont(font);
			painter.setPen(QColor("#f0f3f6"));
			painter.drawText(QRectF(0, size * 0.16, size, size * 0.72), Qt::AlignCenter, "S");
			icon.addPixmap(pixmap);
		}

		return icon;
	}

	QString LauncherMainWindow::IconGlyph(LauncherIcon icon) const
	{
		switch (icon)
		{
		case LauncherIcon::Setup:
			return QChar(0xf0ad);
		case LauncherIcon::Build:
			return QChar(0xf6e3);
		case LauncherIcon::Cook:
			return QChar(0xf466);
		case LauncherIcon::Run:
			return QChar(0xf04b);
		case LauncherIcon::Maintain:
			return QChar(0xf1de);
		case LauncherIcon::Queued:
			return QChar(0xf017);
		case LauncherIcon::Running:
			return QChar(0xf04b);
		case LauncherIcon::Done:
			return QChar(0xf00c);
		case LauncherIcon::Failed:
			return QChar(0xf071);
		case LauncherIcon::Copy:
			return QChar(0xf0c5);
		}

		return QString();
	}

	QIcon LauncherMainWindow::CreateLauncherIcon(LauncherIcon icon, const QColor& color) const
	{
		if (m_iconFontFamily.isEmpty())
		{
			return {};
		}

		QPixmap pixmap(kLauncherIconSize, kLauncherIconSize);
		pixmap.fill(Qt::transparent);

		QPainter painter(&pixmap);
		painter.setRenderHint(QPainter::Antialiasing, true);
		painter.setRenderHint(QPainter::TextAntialiasing, true);
		QFont iconFont(m_iconFontFamily);
		iconFont.setPixelSize(kLauncherIconSize - 1);
		painter.setFont(iconFont);
		painter.setPen(color);
		painter.drawText(pixmap.rect(), Qt::AlignCenter, IconGlyph(icon));
		return QIcon(pixmap);
	}

	QIcon LauncherMainWindow::WorkflowIconForIndex(int workflowIndex) const
	{
		switch (workflowIndex)
		{
		case 0:
			return CreateLauncherIcon(LauncherIcon::Setup, QColor(kColorStateQueued));
		case 1:
			return CreateLauncherIcon(LauncherIcon::Build, QColor(kColorStateQueued));
		case 2:
			return CreateLauncherIcon(LauncherIcon::Cook, QColor(kColorStateQueued));
		case 3:
			return CreateLauncherIcon(LauncherIcon::Run, QColor(kColorStateQueued));
		case 4:
			return CreateLauncherIcon(LauncherIcon::Maintain, QColor(kColorStateQueued));
		default:
			return {};
		}
	}

	QIcon LauncherMainWindow::ActivityIconForState(RunState state) const
	{
		switch (state)
		{
		case RunState::Queued:
			return CreateLauncherIcon(LauncherIcon::Queued, QColor(kColorStateQueued));
		case RunState::Running:
			return CreateLauncherIcon(LauncherIcon::Running, QColor(kColorStateRunning));
		case RunState::Done:
			return CreateLauncherIcon(LauncherIcon::Done, QColor(kColorStateSuccess));
		case RunState::Failed:
			return CreateLauncherIcon(LauncherIcon::Failed, QColor(kColorStateDestructive));
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
		if (m_runButton == nullptr)
		{
			return;
		}

		if (m_selectedOperationId.isEmpty())
		{
			const QString reason = "Select a workflow before running.";
			m_runButton->setEnabled(false);
			m_runButton->setToolTip(reason);
			m_runButton->setAccessibleDescription(reason);
			return;
		}

		if (OperationNeedsProject(m_selectedOperationId) && m_projectModel.SelectedProjectId().isEmpty())
		{
			const QString reason = "No project discovered. Run Sync Third Parties or Check Dependencies, then retry.";
			m_runButton->setEnabled(false);
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

	const LauncherOperationDescriptor* LauncherMainWindow::FindOperationDescriptor(const QString& operationId) const
	{
		for (const LauncherOperationDescriptor& operation : m_backend.Operations())
		{
			if (operation.Id == operationId)
			{
				return &operation;
			}
		}
		return nullptr;
	}

	QString LauncherMainWindow::DisplayNameForOperation(const QString& operationId) const
	{
		const LauncherOperationDescriptor* operation = FindOperationDescriptor(operationId);
		return operation == nullptr ? operationId : operation->DisplayName;
	}

	bool LauncherMainWindow::OperationNeedsProject(const QString& operationId) const
	{
		if (operationId == "workspace.clean")
		{
			return m_settings.CleanScope().contains("selected-cooked");
		}

		return operationId.startsWith("project.") || operationId.startsWith("cook.");
	}

	bool LauncherMainWindow::OperationNeedsConfirmation(const QString& operationId) const
	{
		if (operationId.startsWith("cook."))
		{
			return m_settings.ForceRecook() && !m_settings.ConfirmForceRecook();
		}
		if (operationId == "workspace.clean")
		{
			return false;
		}

		return false;
	}

	QString LauncherMainWindow::FailureRecoveryHint(const QString& operationId, const QString& statusText) const
	{
		if (OperationNeedsProject(operationId) && m_projectModel.SelectedProjectId().isEmpty())
		{
			return "No project is selected. Run Sync Third Parties, then retry this workflow.";
		}
		if (operationId.startsWith("cook.") && OperationNeedsConfirmation(operationId))
		{
			return "Enable Confirm clean cook, then retry.";
		}
		if (operationId.startsWith("project.build") || statusText.contains("cmake", Qt::CaseInsensitive) || statusText.contains("MSBuild", Qt::CaseInsensitive) || statusText.contains("tool", Qt::CaseInsensitive))
		{
			return "Run Check Dependencies, then retry this workflow.";
		}
		if (statusText.contains("Rider", Qt::CaseInsensitive))
		{
			return "Install Rider or switch the IDE selector back to Visual Studio, then retry.";
		}
		if (statusText.contains("shader package", Qt::CaseInsensitive) || statusText.contains("shader", Qt::CaseInsensitive))
		{
			return "Run Cook > Cook Shaders, then retry this workflow.";
		}
		if (statusText.contains("executable is missing", Qt::CaseInsensitive) || statusText.contains("missing", Qt::CaseInsensitive))
		{
			const bool runtimeLaunch = operationId == "project.open.runtime" || ((operationId == "project.run" || operationId == "project.run.smoke") && m_settings.LaunchTarget() == "runtime");
			if ((operationId == "project.open.runtime" || operationId == "project.run" || operationId == "project.run.smoke") && runtimeLaunch)
			{
				return "Run Build > Build Runtime, then retry this workflow.";
			}
			if (FindLaunchOperationDefinition(operationId.toStdString()).has_value())
			{
				return "Run Build > Build Editor, then retry this workflow.";
			}
		}

		if (operationId.startsWith("cook."))
		{
			return "Review the output below. If tools or cooked inputs are missing, run Build Cook Tools before retrying.";
		}

		if (FindLaunchOperationDefinition(operationId.toStdString()).has_value())
		{
			return "Review the output below. If binaries are missing, build the matching target before retrying.";
		}

		return "Review the output below, adjust the selected options, then retry.";
	}

	LauncherOperationRequest LauncherMainWindow::BuildOperationRequest(const QString& operationId) const
	{
		LauncherOperationRequest request;
		request.RepositoryRoot = m_repositoryRoot;
		request.OperationId = operationId;
		request.ProjectId = m_projectModel.SelectedProjectId();
		request.EditorProfile = m_settings.EditorProfile();
		request.RuntimeProfile = m_settings.RuntimeProfile();
		request.WorkspaceIde = m_settings.WorkspaceIde();
		request.SelectedTargets = m_settings.SelectedTargets();
		request.ShaderPackages = m_settings.ShaderPackages();
		request.LaunchBackend = m_settings.LaunchBackend();
		request.LaunchTarget = m_settings.LaunchTarget();
		request.LaunchVSync = m_settings.LaunchVSync();
		request.LaunchHighPerformanceAdapter = m_settings.LaunchHighPerformanceAdapter();
		request.LaunchMeshAutoBatching = m_settings.LaunchMeshAutoBatching();
		request.LaunchCommandLineArguments = m_settings.LaunchCommandLineArguments();
		request.LaunchCVars = m_settings.LaunchCVars();
		request.SmokeBackend = m_settings.SmokeBackend();
		request.SmokeFrameLimit = m_settings.SmokeFrameLimit();
		request.FormatMode = "apply";
		request.CleanScope = m_settings.CleanScope();
		request.LaunchSmokeTest = m_settings.LaunchSmokeTest();
		request.ForceConfigure = m_settings.ForceConfigure();
		request.ForceRecook = m_settings.ForceRecook();
		request.ConfirmForceRecook = m_settings.ConfirmForceRecook();
		request.ConfirmClean = m_settings.ConfirmClean();
		request.SmokeTrace = m_settings.SmokeTrace();
		request.SmokeSkipLevelSwitching = m_settings.SmokeSkipLevelSwitching();
		if (operationId == "project.open.editor")
		{
			request.LaunchTarget = "editor";
			request.LaunchSmokeTest = false;
		}
		else if (operationId == "project.open.runtime")
		{
			request.LaunchTarget = "runtime";
			request.LaunchSmokeTest = false;
		}
		else if (operationId == "project.run.smoke")
		{
			request.LaunchSmokeTest = true;
		}
		return request;
	}

	bool LauncherMainWindow::ConfirmRunRequest(LauncherOperationRequest& request) const
	{
		const bool cleanRequested = request.OperationId == "workspace.clean";
		const bool destructiveRequested = request.ForceRecook || cleanRequested;
		if (!destructiveRequested)
		{
			return true;
		}
		if (request.ForceRecook && !request.ConfirmForceRecook)
		{
			QMessageBox::warning(
			    const_cast<LauncherMainWindow*>(this),
			    "Confirmation Required",
			    "Enable Confirm clean cook before removing cooked outputs.");
			return false;
		}
		if (cleanRequested && !request.ConfirmClean)
		{
			QStringList scopeNames;
			for (const QString& scopeValue : request.CleanScope.split(QRegularExpression("[,;\\n]"), Qt::SkipEmptyParts))
			{
				scopeNames.push_back(CleanScopeDisplayName(scopeValue));
			}

			QString message = "Clean scopes:\n" + scopeNames.join('\n');
			if (!request.ProjectId.isEmpty())
			{
				message += "\nProject: " + request.ProjectId;
			}
			message += "\n\nThis removes generated files for the selected scope. Continue?";
			const QMessageBox::StandardButton result = QMessageBox::question(
			    const_cast<LauncherMainWindow*>(this),
			    "Confirm Clean Workspace",
			    message,
			    QMessageBox::Ok | QMessageBox::Cancel,
			    QMessageBox::Cancel);
			if (result != QMessageBox::Ok)
			{
				return false;
			}

			request.ConfirmClean = true;
			return true;
		}

		const QMessageBox::StandardButton result = QMessageBox::question(
		    const_cast<LauncherMainWindow*>(this),
		    "Confirm Clean Cook",
		    "This workflow will remove cooked outputs before cooking. Continue?",
		    QMessageBox::Yes | QMessageBox::No,
		    QMessageBox::No);
		return result == QMessageBox::Yes;
	}

	void LauncherMainWindow::PromptForLauncherRestart()
	{
		const QMessageBox::StandardButton result = QMessageBox::question(
		    this,
		    "Launcher Rebuilt",
		    "Sparkle Launcher was rebuilt successfully. Restart now to run the new binary?",
		    QMessageBox::Yes | QMessageBox::No,
		    QMessageBox::Yes);
		if (result != QMessageBox::Yes)
		{
			SetStatusMessage("Launcher rebuilt. Restart it when you're ready.");
			return;
		}

		const QString executablePath = QCoreApplication::applicationFilePath();
		const bool started = QProcess::startDetached(executablePath, {});
		if (!started)
		{
			QMessageBox::warning(
			    this,
			    "Restart Failed",
			    "The rebuilt launcher is ready, but the restart command could not be started.");
			SetStatusMessage("Launcher rebuilt, but automatic restart failed.");
			return;
		}

		QCoreApplication::quit();
	}

	bool LauncherMainWindow::OfferWorkspacePrerequisiteOperation(const QString& operationId)
	{
		BuildWorkspaceOperationRequest request;
		request.RepositoryRoot = m_repositoryRoot;
		request.ProjectId = m_projectModel.SelectedProjectId().isEmpty() ? std::string("Showcase") : m_projectModel.SelectedProjectId().toStdString();
		request.EditorProfile = m_settings.EditorProfile().toStdString();
		request.RuntimeProfile = m_settings.RuntimeProfile().toStdString();
		request.PreferredIde = SelectedWorkspaceIde(m_settings);
		request.ForceConfigure = m_settings.ForceConfigure();

		const BuildWorkspaceOperationPlan plan = PlanBuildWorkspaceOperation(operationId.toStdString(), request);
		if (plan.CanRun)
		{
			return true;
		}

		QString prerequisiteOperationId;
		QString promptTitle;
		QString promptAction;
		if (!plan.Toolchain.RequiredToolsAvailable)
		{
			prerequisiteOperationId = "toolchain.check";
			promptTitle = "Check Dependencies";
			promptAction = "Required dependencies are missing. Run Check Dependencies now?";
		}
		else if ((operationId == "workspace.open-solution" || operationId == "launcher.build.self" || operationId.startsWith("project.build") || operationId == "cook.tools.prepare") && !plan.Freshness.Current)
		{
			prerequisiteOperationId = "workspace.generate-solution";
			promptTitle = "Regenerate Solution";
			promptAction = "Solution/workspace files are not current. Run Regenerate Solution now?";
		}
		else
		{
			return true;
		}

		QStringList readiness;
		for (const std::string& message : plan.ReadinessMessages)
		{
			readiness.push_back(QString::fromStdString(message));
		}

		const QMessageBox::StandardButton result = QMessageBox::question(
		    this,
		    "Setup Prerequisite Missing",
		    promptAction + (readiness.isEmpty() ? QString() : "\n\n" + readiness.join('\n')),
		    QMessageBox::Ok | QMessageBox::Cancel,
		    QMessageBox::Ok);
		if (result != QMessageBox::Ok)
		{
			return false;
		}

		LauncherOperationRequest prerequisiteRequest = BuildOperationRequest(prerequisiteOperationId);
		if (!ConfirmRunRequest(prerequisiteRequest))
		{
			return false;
		}

		StartOperation(std::move(prerequisiteRequest), promptTitle);
		return false;
	}

	bool LauncherMainWindow::OfferLaunchPrerequisiteOperation(const QString& operationId)
	{
		LauncherOperationRequest request = BuildOperationRequest(operationId);
		LaunchOperationRequest launchRequest;
		launchRequest.RepositoryRoot = request.RepositoryRoot;
		launchRequest.ProjectId = request.ProjectId.toStdString();
		launchRequest.EditorProfile = request.EditorProfile.toStdString();
		launchRequest.RuntimeProfile = request.RuntimeProfile.toStdString();
		launchRequest.Target = request.LaunchTarget.toStdString();
		launchRequest.GraphicsBackend = request.LaunchBackend.toStdString();
		launchRequest.VSync = request.LaunchVSync.toStdString();
		launchRequest.PreferHighPerformanceAdapter = request.LaunchHighPerformanceAdapter.toStdString();
		launchRequest.MeshAutoBatching = request.LaunchMeshAutoBatching.toStdString();
		for (const QString& argument : QProcess::splitCommand(request.LaunchCommandLineArguments))
		{
			if (!argument.isEmpty())
			{
				launchRequest.CustomArguments.push_back(argument.toStdString());
			}
		}
		launchRequest.CustomCVars.clear();
		for (const QString& part : request.LaunchCVars.split(QRegularExpression("[,;\\n]"), Qt::SkipEmptyParts))
		{
			const QString trimmed = part.trimmed();
			if (!trimmed.isEmpty())
			{
				launchRequest.CustomCVars.push_back(trimmed.toStdString());
			}
		}
		launchRequest.SmokeBackend = request.SmokeBackend.toStdString();
		launchRequest.SmokeFrameLimit = request.SmokeFrameLimit.toStdString();
		launchRequest.EnableSmokeTest = request.LaunchSmokeTest;
		launchRequest.SmokeTrace = request.SmokeTrace;
		launchRequest.SmokeSkipLevelSwitching = request.SmokeSkipLevelSwitching;

		const LaunchOperationPlan plan = PlanLaunchOperation(operationId.toStdString(), launchRequest);
		if (plan.CanRun)
		{
			return true;
		}

		bool executableMissing = false;
		bool cookedAssetsMissing = false;
		QStringList readiness;
		for (const std::string& message : plan.ReadinessMessages)
		{
			const QString readinessMessage = QString::fromStdString(message);
			readiness.push_back(readinessMessage);
			executableMissing = executableMissing || readinessMessage.contains("Executable is missing", Qt::CaseInsensitive);
			cookedAssetsMissing = cookedAssetsMissing || readinessMessage.contains("Cooked meshes are missing", Qt::CaseInsensitive) ||
			    readinessMessage.contains("Cooked textures are missing", Qt::CaseInsensitive) ||
			    readinessMessage.contains("Cooked shaders are missing", Qt::CaseInsensitive);
		}

		QString prerequisiteOperationId;
		QString promptTitle;
		QString promptAction;
		if (executableMissing)
		{
			const bool runtimeTarget = request.LaunchTarget == "runtime";
			prerequisiteOperationId = runtimeTarget ? "project.build.runtime" : "project.build.editor";
			promptTitle = runtimeTarget ? "Build Runtime" : "Build Editor";
			promptAction = "The executable is missing. Start " + promptTitle + " now?";
		}
		else if (cookedAssetsMissing)
		{
			prerequisiteOperationId = "cook.project";
			promptTitle = "Cook All Assets";
			promptAction = "Cooked meshes, textures, or shaders are missing. Start Cook All Assets now?";
		}
		else
		{
			return true;
		}

		const QMessageBox::StandardButton result = QMessageBox::question(
		    this,
		    "Launch Prerequisite Missing",
		    promptAction + "\n\n" + readiness.join('\n'),
		    QMessageBox::Ok | QMessageBox::Cancel,
		    QMessageBox::Ok);
		if (result != QMessageBox::Ok)
		{
			SetStatusMessage("Launch canceled");
			return false;
		}

		LauncherOperationRequest prerequisiteRequest = BuildOperationRequest(prerequisiteOperationId);
		if (!ConfirmRunRequest(prerequisiteRequest))
		{
			SetStatusMessage("Prerequisite run canceled");
			return false;
		}

		StartOperation(std::move(prerequisiteRequest), DisplayNameForOperation(prerequisiteOperationId));
		return false;
	}

	bool LauncherMainWindow::OfferCookPrerequisiteOperation(const QString& operationId)
	{
		LauncherOperationRequest request = BuildOperationRequest(operationId);
		CookOperationRequest cookRequest;
		cookRequest.RepositoryRoot = request.RepositoryRoot;
		cookRequest.ProjectId = request.ProjectId.toStdString();
		cookRequest.RuntimeProfile = request.RuntimeProfile.toStdString();
		cookRequest.Mode = request.ForceRecook ? CookMode::Force : CookMode::Incremental;
		cookRequest.ForceRecookConfirmed = request.ConfirmForceRecook;
		for (const QString& part : request.ShaderPackages.split(QRegularExpression("[,;\\n]"), Qt::SkipEmptyParts))
		{
			const QString trimmed = part.trimmed();
			if (!trimmed.isEmpty())
			{
				cookRequest.ShaderPackages.push_back(trimmed.toStdString());
			}
		}

		const CookOperationPlan plan = PlanCookOperation(operationId.toStdString(), cookRequest);
		if (plan.CanRun)
		{
			return true;
		}

		bool workspaceMissing = false;
		bool cookToolsMissing = false;
		QStringList readiness;
		for (const std::string& message : plan.ReadinessMessages)
		{
			const QString readinessMessage = QString::fromStdString(message);
			readiness.push_back(readinessMessage);
			workspaceMissing = workspaceMissing || readinessMessage.contains("Run Regenerate Solution first", Qt::CaseInsensitive);
			cookToolsMissing = cookToolsMissing || readinessMessage.contains("run Build Cook Tools first", Qt::CaseInsensitive);
		}

		QString prerequisiteOperationId;
		QString promptTitle;
		QString promptAction;
		if (workspaceMissing)
		{
			prerequisiteOperationId = "workspace.generate-solution";
			promptTitle = "Regenerate Solution";
			promptAction = "Solution/workspace files are not current. Run Regenerate Solution now?";
		}
		else if (cookToolsMissing)
		{
			prerequisiteOperationId = "cook.tools.prepare";
			promptTitle = "Build Cook Tools";
			promptAction = "Required cook tools are missing. Run Build Cook Tools now?";
		}
		else
		{
			return true;
		}

		const QMessageBox::StandardButton result = QMessageBox::question(
		    this,
		    "Cook Prerequisite Missing",
		    promptAction + "\n\n" + readiness.join('\n'),
		    QMessageBox::Ok | QMessageBox::Cancel,
		    QMessageBox::Ok);
		if (result != QMessageBox::Ok)
		{
			return false;
		}

		LauncherOperationRequest prerequisiteRequest = BuildOperationRequest(prerequisiteOperationId);
		if (!ConfirmRunRequest(prerequisiteRequest))
		{
			return false;
		}

		StartOperation(std::move(prerequisiteRequest), DisplayNameForOperation(prerequisiteOperationId));
		return false;
	}

	void LauncherMainWindow::StartOperation(LauncherOperationRequest request, const QString& title)
	{
		request.RunId = QStringLiteral("run-%1").arg(++m_nextRunIndex, 4, 10, QChar('0'));
		RegisterRun(request.RunId, title);
		SetStatusMessage("Starting " + title);
		m_backend.RunOperation(std::move(request));
	}

	void LauncherMainWindow::SetStatusMessage(const QString& message)
	{
		Q_UNUSED(message);
	}

	void LauncherMainWindow::SetSelectedOperation(const QString& operationId)
	{
		m_selectedOperationId = operationId;
		const QString title = DisplayNameForOperation(operationId);
		SetControlsEnabled(true);
		if (m_activeOperationLabel != nullptr)
		{
			m_activeOperationLabel->setText(title);
		}
		if (m_runButton != nullptr)
		{
			m_runButton->setText("Run");
		}
		if (m_optionsStack != nullptr && m_optionsPageByOperation.contains(operationId))
		{
			m_optionsStack->setCurrentIndex(m_optionsPageByOperation.value(operationId));
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
		}

		UpdateRunAvailability();
	}

	void LauncherMainWindow::RegisterRun(const QString& runId, const QString& title)
	{
		++m_startedRunCount;
		QListWidgetItem* item = new QListWidgetItem(m_activityList);
		item->setData(Qt::UserRole, runId);
		m_runItems.insert(runId, item);
		m_runTitles.insert(runId, title);
		SetRunState(runId, RunState::Queued, title);
		m_runOutputs.insert(runId, title + " queued.\n");
		m_activityList->setCurrentItem(item);
		m_activeRunId = runId;
		UpdateProgress();
	}

	void LauncherMainWindow::SetRunState(const QString& runId, RunState state, const QString& title)
	{
		QListWidgetItem* item = m_runItems.value(runId, nullptr);
		if (item == nullptr)
		{
			return;
		}

		m_runStates.insert(runId, state);
		m_runTitles.insert(runId, title);

		QString stateText;
		QColor stateColor;
		switch (state)
		{
		case RunState::Queued:
			stateText = "Queued";
			stateColor = QColor(kColorStateQueued);
			break;
		case RunState::Running:
			stateText = "Running";
			stateColor = QColor(kColorStateRunning);
			break;
		case RunState::Done:
			stateText = "Done";
			stateColor = QColor(kColorStateSuccess);
			break;
		case RunState::Failed:
			stateText = "Failed";
			stateColor = QColor(kColorStateDestructive);
			break;
		}

		item->setText(stateText + ": " + title);
		item->setIcon(ActivityIconForState(state));
		item->setData(Qt::UserRole + 1, stateText);
		item->setData(Qt::AccessibleTextRole, stateText + ": " + title);
		item->setData(Qt::AccessibleDescriptionRole, "Launcher activity run " + stateText.toLower());
		item->setForeground(QBrush(stateColor));
		item->setToolTip(stateText + ": " + title);
	}

	void LauncherMainWindow::AppendRunOutput(const QString& runId, const QString& text)
	{
		QString output = m_runOutputs.value(runId);
		output += text;
		const int overflowCharacters = output.size() - kMaxOperationOutputCharacters;
		if (overflowCharacters > 0)
		{
			output.remove(0, overflowCharacters);
		}
		m_runOutputs.insert(runId, output);
	}

	void LauncherMainWindow::ShowRunOutput(const QString& runId)
	{
		m_activeRunId = runId;
		const RunState state = m_runStates.value(runId, RunState::Queued);
		const QString title = m_runTitles.value(runId, "Selected run");
		if (m_selectedRunSummary != nullptr)
		{
			switch (state)
			{
			case RunState::Queued:
				m_selectedRunSummary->setText("Queued: " + title + ". Waiting to start.");
				break;
			case RunState::Running:
				m_selectedRunSummary->setText("Running: " + title + ". Output is updating below.");
				break;
			case RunState::Done:
				m_selectedRunSummary->setText("Done: " + title + ". Output is available below.");
				break;
			case RunState::Failed:
				m_selectedRunSummary->setText("Failed: " + title + ". Review the summary and raw output below.");
				break;
			}
		}
		if (m_operationOutput != nullptr)
		{
			const bool compactOutput = state == RunState::Done;
			m_operationOutput->setMinimumHeight(compactOutput ? kOperationOutputMinHeight : kOperationOutputProminentMinHeight);
			m_operationOutput->setMaximumHeight(compactOutput ? kOperationOutputCompactMaxHeight : kOperationOutputMaxHeight);
		}
		m_operationOutput->setPlainText(m_runOutputs.value(runId));
		m_operationOutput->moveCursor(QTextCursor::End);
		if (m_copyOutputButton != nullptr)
		{
			const bool canCopyOutput = !m_operationOutput->toPlainText().isEmpty();
			m_copyOutputButton->setEnabled(canCopyOutput);
			m_copyOutputButton->setToolTip(canCopyOutput ? "Copy output for the selected run. Shortcut: Ctrl+Shift+C." : "Select a run to copy its output. Shortcut: Ctrl+Shift+C.");
		}
	}

	void LauncherMainWindow::UpdateProgress()
	{
		if (m_progressBar == nullptr || m_progressLabel == nullptr)
		{
			return;
		}

		int queuedCount = 0;
		int runningCount = 0;
		int succeededCount = 0;
		int failedCount = 0;
		for (const RunState state : m_runStates)
		{
			switch (state)
			{
			case RunState::Queued:
				++queuedCount;
				break;
			case RunState::Running:
				++runningCount;
				break;
			case RunState::Done:
				++succeededCount;
				break;
			case RunState::Failed:
				++failedCount;
				break;
			}
		}

		const bool hasRuns = m_startedRunCount > 0;
		if (m_activityDetailsPanel != nullptr)
		{
			m_activityDetailsPanel->setVisible(hasRuns);
		}
		if (m_progressBar != nullptr)
		{
			m_progressBar->setVisible(hasRuns);
		}
		if (m_copyOutputButton != nullptr && !hasRuns)
		{
			m_copyOutputButton->setEnabled(false);
		}
		if (m_startedRunCount == 0)
		{
			m_progressBar->setRange(0, 1);
			m_progressBar->setValue(0);
			m_progressBar->setFormat(QString());
			m_progressLabel->setText("No runs yet");
			return;
		}

		m_progressBar->setRange(0, m_startedRunCount);
		m_progressBar->setValue(succeededCount + failedCount);
		m_progressBar->setFormat(QString());

		QStringList activityParts;
		if (queuedCount > 0)
		{
			activityParts.push_back(QStringLiteral("%1 queued").arg(queuedCount));
		}
		if (runningCount > 0)
		{
			activityParts.push_back(QStringLiteral("%1 running").arg(runningCount));
		}
		if (failedCount > 0)
		{
			activityParts.push_back(QStringLiteral("%1 failed").arg(failedCount));
		}
		if (succeededCount > 0)
		{
			activityParts.push_back(QStringLiteral("%1 done").arg(succeededCount));
		}
		if (activityParts.empty())
		{
			m_progressLabel->setText("No active runs");
		}
		else
		{
			m_progressLabel->setText(activityParts.join(", "));
		}
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
		RebuildOptionsPages();
		UpdateRunAvailability();
	}

	void LauncherMainWindow::PopulateProjectCombo(QComboBox& combo) const
	{
		const QSignalBlocker blocker(&combo);
		combo.clear();
		if (m_projectModel.Projects().empty())
		{
			combo.addItem("No projects found", "");
			combo.setToolTip("No projects were discovered in the repository. Run Sync Third Parties or inspect project discovery output.");
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

	QVector<LauncherMainWindow::WorkflowDefinition> LauncherMainWindow::CreateWorkflowDefinitions() const
	{
		return {
		    {"Setup", "Inspect and configure", {"toolchain.check", "workspace.setup", "workspace.generate-solution", "workspace.clean"}},
		    {"Build", "Compile targets", {"launcher.build.self", "project.build.editor", "project.build.runtime", "cook.tools.prepare"}},
		    {"Cook", "Prepare content", {"cook.project", "cook.shaders", "cook.textures", "cook.assets"}},
		    {"Run", "Open targets", {"workspace.open-solution", "project.open.editor", "project.open.runtime", "project.run.smoke", "quality.format"}},
		};
	}

	void LauncherMainWindow::ApplyVisualStyle()
	{
		const QString background = "#2b2b2b";
		const QString shell = "#242424";
		const QString panel = "#343434";
		const QString panelRaised = "#3a3a3a";
		const QString panelHover = "#3f3f3f";
		const QString field = "#262626";
		const QString border = "#1b1b1b";
		const QString borderSoft = "#454545";
		const QString borderStrong = "#5b5b5b";
		const QString divider = "#191919";
		const QString focus = "#d0d7de";
		const QString primary = "#1479c9";
		const QString primaryHover = "#2388da";
		const QString selection = "#0f6fb9";
		const QString accent = "#76b900";
		const QString warning = QString::fromLatin1(kColorStateWarning);
		const QString destructive = QString::fromLatin1(kColorStateDestructive);
		const QString textPrimary = "#f4f4f4";
		const QString textBody = "#dddddd";
		const QString textSecondary = "#c1c1c1";
		const QString textMuted = "#9a9a9a";

		QString style;
		const auto addRule = [&style](const QString& selector, const QString& body) {
			style += selector + " { " + body + " }";
		};

		addRule("QMainWindow, QWidget", "background: " + background + "; color: " + textBody + "; font-family: 'Segoe UI'; font-size: 10pt;");
		addRule("QLabel", "color: " + textBody + "; background: transparent;");
		addRule("#WorkflowSurface", "background: " + background + ";");
		addRule("#ProcessPanel", "background: qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 " + panelRaised + ", stop:1 " + shell + "); border: 1px solid " + border + "; border-top-color: " + borderSoft + "; padding: 0;");
		addRule("#OptionsPanel", "background: qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 " + panelRaised + ", stop:1 " + panel + "); border: 1px solid " + border + "; border-top-color: " + borderSoft + "; border-radius: 2px;");
		addRule("#OutputPanel", "background: qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 #2f2f2f, stop:1 " + shell + "); border-top: 1px solid " + border + ";");
		addRule("#FooterContextPanel", "background: qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 #2f2f2f, stop:1 " + shell + "); border-top: 1px solid " + border + "; padding: 6px 14px 8px 14px;");
		addRule("#FooterFieldLabel", "color: " + textMuted + "; font-size: 8.5pt; font-weight: 600;");
		addRule("#FooterContextCombo", "background: " + field + "; border: 1px solid " + border + "; border-top-color: " + borderStrong + "; border-radius: 2px; padding: 3px 8px; color: " + textBody + "; min-height: 24px; max-height: 28px; font-size: 8.5pt;");
		addRule("#FooterContextCombo:focus", "border: 1px solid " + focus + ";");
		addRule("#OptionsScrollArea, #OptionsStack, #OptionsContent, #OperationStack, #InlineOptionsSection, #ActivityDetailsPanel", "background: transparent; border: none;");
		addRule("#OptionsScrollArea QWidget", "background: transparent;");
		addRule("#OptionRow", "background: transparent; min-height: 36px;");
		addRule("#OptionGroup", "background: qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 #303030, stop:1 #292929); border: 1px solid " + border + "; border-top-color: " + borderStrong + "; border-radius: 2px; margin-top: 8px;");

		addRule("#ActiveOperationLabel", "color: " + textPrimary + "; font-size: 15pt; font-weight: 700;");
		addRule("#WorkflowRailTitle", "color: " + textPrimary + "; font-size: 11pt; font-weight: 700; padding: 0 0 2px 0;");
		addRule("#SectionLabel", "color: " + textPrimary + "; font-size: 10.5pt; font-weight: 700; padding-top: 2px;");
		addRule("#OptionGroupTitle", "color: " + textPrimary + "; font-size: 10pt; font-weight: 700; padding: 0 0 0 6px; border-left: 3px solid " + accent + ";");
		addRule("#FieldLabel", "color: " + textSecondary + "; font-size: 9pt; font-weight: 600; padding-top: 0;");
		addRule("#OptionHelpText", "color: " + textMuted + "; font-size: 8.5pt; line-height: 125%;");
		addRule("#StatusRow", "background: #262626; border: 1px solid #1f1f1f; border-top-color: #3b3b3b; padding: 7px 9px;");
		addRule("#StatusLabel", "color: " + textBody + "; font-size: 9pt; font-weight: 650;");
		addRule("#StatusValue", "color: " + textMuted + "; font-size: 8.5pt; font-weight: 700; padding: 2px 7px; border: 1px solid #3d3d3d; background: #303030;");
		addRule("#StatusValue[State=\"ok\"]", "color: #dff3cf; border-color: #4d6f29; background: #2b3522;");
		addRule("#StatusValue[State=\"warning\"]", "color: #ffe2a8; border-color: #7a5a23; background: #3a3123;");
		addRule("#StatusValue[State=\"bad\"]", "color: #ffd0cc; border-color: #79413d; background: #3a2928;");
		addRule("#StatusDetail", "color: " + textMuted + "; font-size: 8.5pt;");
		addRule("#ActionRow", "background: #262626; border: 1px solid #1f1f1f; border-top-color: #3b3b3b; padding: 8px 9px;");
		addRule("#ActionTitle", "color: " + textPrimary + "; font-size: 9pt; font-weight: 700;");
		addRule("#InlineActionButton", "background: qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 #4b4b4b, stop:1 #3b3b3b); color: " + textBody + "; border: 1px solid " + border + "; border-top-color: " + borderStrong + "; padding: 6px 10px; min-width: 120px;");
		addRule("#InlineActionButton:hover", "background: " + panelHover + ";");
		addRule("#MutedLabel", "color: " + textMuted + "; padding: 6px 0;");
		addRule("#ProgressLabel", "color: " + textPrimary + "; font-size: 10.5pt; font-weight: 700;");
		addRule("#ActivitySummary", "color: " + textSecondary + "; background: transparent; font-size: 9pt; font-weight: 600; padding: 0 0 2px 4px;");

		addRule("#WorkflowGroupButton", "background: transparent; color: " + textMuted + "; border: 1px solid transparent; border-radius: 2px; padding: 7px 9px; text-align: left; font-size: 9pt; font-weight: 650; min-width: 78px;");
		addRule("#WorkflowGroupButton:hover", "background: " + panel + "; color: " + textBody + "; border: 1px solid " + borderSoft + ";");
		addRule("#WorkflowGroupButton:pressed", "background: #3f3f3f; color: " + textPrimary + "; border: 1px solid " + borderStrong + "; border-left: 3px solid " + accent + "; padding-left: 7px;");
		addRule("#WorkflowGroupButton[ActiveState=\"true\"]", "background: #3a3a3a; color: " + textPrimary + "; border: 1px solid " + borderStrong + "; border-left: 3px solid " + accent + "; padding-left: 7px;");
		addRule("#WorkflowGroupButton:focus", "border: 1px solid " + focus + "; color: " + textPrimary + ";");
		addRule("#WorkflowButton", "background: transparent; color: " + textBody + "; border: 1px solid transparent; border-radius: 2px; padding: 8px 10px; text-align: left; font-size: 10pt; font-weight: 600;");
		addRule("#WorkflowButton:hover", "background: " + panelHover + "; border: 1px solid " + border + ";");
		addRule("#WorkflowButton:checked", "background: " + selection + "; border: 1px solid #48a2df; color: #ffffff;");
		addRule("#WorkflowButton:focus", "border: 1px solid " + focus + "; color: " + textPrimary + ";");

		addRule("QPushButton", "background: qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 " + primaryHover + ", stop:1 " + primary + "); color: #ffffff; border: 1px solid #0c4f86; border-top-color: #5aaae4; border-radius: 2px; padding: 8px 14px; font-weight: 650;");
		addRule("QPushButton:hover", "background: " + primaryHover + ";");
		addRule("QPushButton:focus", "border: 1px solid " + focus + ";");
		addRule("QPushButton:disabled", "background: #3a3a3a; border: 1px solid " + border + "; color: " + textMuted + ";");
		addRule("#PrimaryActionButton", "background: " + primary + "; min-width: 96px;");
		addRule("#PrimaryActionButton:hover", "background: " + primaryHover + ";");
		addRule("#SecondaryButton", "background: qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 #4b4b4b, stop:1 #3b3b3b); color: " + textBody + "; border: 1px solid " + border + "; border-top-color: " + borderStrong + ";");
		addRule("#SecondaryButton:hover", "background: " + panelHover + ";");
		addRule("#SecondaryButton:focus", "border: 1px solid " + focus + "; color: " + textPrimary + ";");
		addRule("QComboBox, QLineEdit, QTextEdit", "background: " + field + "; border: 1px solid " + border + "; border-top-color: " + borderStrong + "; border-radius: 2px; padding: 7px 9px; color: " + textBody + "; selection-background-color: " + selection + ";");
		addRule("QComboBox:focus, QLineEdit:focus, QTextEdit:focus", "border: 1px solid " + focus + ";");
		addRule("QComboBox:disabled", "background: " + shell + "; border: 1px solid " + border + "; color: " + textMuted + ";");
		addRule("QCheckBox", "spacing: 8px; padding: 3px 0; color: " + textBody + ";");
		addRule("QCheckBox:focus", "border: 1px solid " + focus + "; border-radius: 2px; color: " + textPrimary + ";");
		addRule("QCheckBox:disabled", "color: " + textMuted + ";");
		addRule("#WarningCheckBox", "color: " + warning + ";");
		addRule("#DestructiveCheckBox", "color: " + destructive + ";");

		addRule("QListWidget", "background: transparent; border: none; border-radius: 0; padding: 0; outline: 0;");
		addRule("QListWidget:focus", "border: 1px solid " + focus + ";");
		addRule("QListWidget::item", "padding: 8px 10px; border-radius: 2px; color: " + textBody + ";");
		addRule("QListWidget::item:selected", "background: " + selection + "; color: #ffffff;");
		addRule("#ActivityDetailsPanel", "background: #303030; border: 1px solid " + border + "; border-top-color: " + borderSoft + ";");
		addRule("#ActivityList", "background: #282828; border: 1px solid " + border + "; border-top-color: " + borderSoft + "; border-radius: 0; padding: 0;");
		addRule("#OperationOutput", "background: transparent; border: none; border-radius: 0; padding: 4px 0 0 4px; font-family: 'Cascadia Mono'; font-size: 9pt;");
		addRule("QProgressBar", "background: #202020; border: 1px solid " + border + "; border-radius: 0; color: " + textBody + "; text-align: center; min-height: 6px; max-height: 6px;");
		addRule("QProgressBar::chunk", "background: " + accent + "; border-radius: 0;");
		setStyleSheet(style);
	}
}
