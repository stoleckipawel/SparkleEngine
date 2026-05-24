#include "LauncherMainWindow.h"

#include "LauncherBackend.h"
#include "LauncherProjectModel.h"
#include "LauncherSettings.h"

#include <QtCore/QSignalBlocker>
#include <QtCore/Qt>
#include <QtGui/QBrush>
#include <QtGui/QClipboard>
#include <QtGui/QColor>
#include <QtGui/QFont>
#include <QtGui/QFontDatabase>
#include <QtGui/QGuiApplication>
#include <QtGui/QPainter>
#include <QtGui/QPixmap>
#include <QtGui/QTextCursor>
#include <QtGui/QTextDocument>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QScrollArea>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QWidget>

#include <utility>

namespace SparkleLauncher
{
	static constexpr int kMaxOperationOutputCharacters = 1000000;
	static constexpr int kSpaceTiny = 2;
	static constexpr int kSpaceSmall = 8;
	static constexpr int kSpaceMedium = 12;
	static constexpr int kSpaceLarge = 16;
	static constexpr int kPanelHorizontalMargin = 22;
	static constexpr int kPanelVerticalMargin = 18;
	static constexpr int kWorkflowRailWidth = 346;
	static constexpr int kWorkflowGroupMinHeight = 30;
	static constexpr int kWorkflowButtonMinHeight = 32;
	static constexpr int kFieldLabelWidth = 116;
	static constexpr int kActivityListWidth = 280;
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
		statusBar()->showMessage("Ready");
		LoadLauncherIconFont();

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
		connect(&m_backend, &LauncherBackend::OperationStarted, this, &LauncherMainWindow::DisplayOperationStarted);
		connect(&m_backend, &LauncherBackend::OperationOutputReceived, this, &LauncherMainWindow::AppendOperationOutput);
		connect(&m_backend, &LauncherBackend::OperationFinished, this, &LauncherMainWindow::DisplayOperationFinished);

		UpdateProgress();
		RefreshProjects();
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
		SetStatusMessage("Copied activity output");
	}

	void LauncherMainWindow::RunSelectedOperation()
	{
		if (m_selectedOperationId.isEmpty())
		{
			if (m_operationOutput != nullptr)
			{
				m_operationOutput->setPlainText("Choose a workflow before running.");
			}
			SetStatusMessage("No process selected");
			return;
		}

		if (OperationNeedsProject(m_selectedOperationId) && m_projectModel.SelectedProjectId().isEmpty())
		{
			const QString message = "No project discovered. Run Setup Workspace or Check Toolchain, then retry.";
			if (m_operationOutput != nullptr)
			{
				m_operationOutput->setPlainText(message);
			}
			SetStatusMessage(message);
			return;
		}

		LauncherOperationRequest request = BuildOperationRequest(m_selectedOperationId);
		if (!ConfirmRunRequest(request))
		{
			SetStatusMessage("Process canceled");
			return;
		}

		const QString title = DisplayNameForOperation(m_selectedOperationId);
		request.RunId = QStringLiteral("run-%1").arg(++m_nextRunIndex, 4, 10, QChar('0'));
		RegisterRun(request.RunId, title);
		SetStatusMessage("Starting " + title);
		m_backend.RunOperation(std::move(request));
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
	}

	QWidget* LauncherMainWindow::CreateWorkflowSurface()
	{
		QFrame* surface = new QFrame(this);
		surface->setObjectName("WorkflowSurface");
		QHBoxLayout* layout = new QHBoxLayout(surface);
		layout->setContentsMargins(kSpaceLarge, kSpaceLarge, kSpaceLarge, kSpaceLarge);
		layout->setSpacing(kSpaceLarge);
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
			groupButton->setCheckable(true);
			groupButton->setMinimumHeight(kWorkflowGroupMinHeight);
			groupButton->setProperty("WorkflowIndex", workflowIndex);
			groupButton->setToolTip(workflow.Subtitle);
			groupButton->setAccessibleName(workflow.Title + " workflow group");
			groupButton->setIcon(WorkflowIconForIndex(workflowIndex));
			groupButton->setIconSize(QSize(kLauncherIconSize, kLauncherIconSize));
			RegisterFocusable(groupButton);
			m_workflowGroupButtonGroup->addButton(groupButton);
			groupLayout->addWidget(groupButton);
			if (workflowIndex == 0)
			{
				groupButton->setChecked(true);
			}

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
		button->setToolTip(DescriptionForOperation(operationId));
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
		layout->addWidget(m_activeOperationLabel);

		m_activeOperationDescription = new QLabel("Choose a workflow from the left.", panel);
		m_activeOperationDescription->setObjectName("OperationDescription");
		m_activeOperationDescription->setWordWrap(true);
		layout->addWidget(m_activeOperationDescription);

		m_optionsStack = new QStackedWidget(panel);
		m_optionsStack->setObjectName("OptionsStack");
		for (const WorkflowDefinition& workflow : CreateWorkflowDefinitions())
		{
			for (const QString& operationId : workflow.OperationIds)
			{
				if (m_optionsPageByOperation.contains(operationId))
				{
					continue;
				}

				const int pageIndex = m_optionsStack->addWidget(CreateOptionsPage(operationId, panel));
				m_optionsPageByOperation.insert(operationId, pageIndex);
			}
		}
		layout->addWidget(m_optionsStack, 1);
		m_optionsStack->setVisible(false);

		QHBoxLayout* actionLayout = new QHBoxLayout();
		actionLayout->setSpacing(kSpaceSmall + kSpaceTiny);
		actionLayout->addStretch(1);
		m_runButton = new QPushButton("Run", panel);
		m_runButton->setObjectName("PrimaryActionButton");
		m_runButton->setIcon(CreateLauncherIcon(LauncherIcon::Run, QColor("#ffffff")));
		m_runButton->setIconSize(QSize(kLauncherIconSize, kLauncherIconSize));
		m_runButton->setToolTip("Start this process. Other running processes keep running.");
		m_runButton->setEnabled(false);
		m_runButton->setAccessibleName("Run selected workflow");
		RegisterFocusable(m_runButton);
		connect(m_runButton, &QPushButton::clicked, this, &LauncherMainWindow::RunSelectedOperation);
		actionLayout->addWidget(m_runButton);
		layout->addLayout(actionLayout);
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
		m_progressLabel = new QLabel("No processes running", panel);
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
		m_copyOutputButton->setAccessibleName("Copy selected activity output");
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
		RegisterFocusable(m_activityList);
		connect(m_activityList, &QListWidget::currentItemChanged, this, &LauncherMainWindow::DisplaySelectedRunOutput);
		activityLayout->addWidget(m_activityList, 1);

		QVBoxLayout* outputLayout = new QVBoxLayout();
		outputLayout->setContentsMargins(0, 0, 0, 0);
		outputLayout->setSpacing(kSpaceSmall);
		m_selectedRunSummary = new QLabel("Select an activity to view output.", panel);
		m_selectedRunSummary->setObjectName("ActivitySummary");
		m_selectedRunSummary->setWordWrap(true);
		outputLayout->addWidget(m_selectedRunSummary);

		m_operationOutput = new QTextEdit(panel);
		m_operationOutput->setObjectName("OperationOutput");
		m_operationOutput->setReadOnly(true);
		m_operationOutput->setMinimumHeight(kOperationOutputMinHeight);
		m_operationOutput->setMaximumHeight(kOperationOutputMaxHeight);
		m_operationOutput->setToolTip("Select an activity to view its output.");
		m_operationOutput->setAccessibleName("Selected activity output");
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
		return label;
	}

	QLabel* LauncherMainWindow::CreateFieldLabel(const QString& title) const
	{
		QLabel* label = new QLabel(title);
		label->setObjectName("FieldLabel");
		return label;
	}

	QCheckBox* LauncherMainWindow::CreateBoundCheckBox(const QString& label, const QString& tooltip, bool checked, void (LauncherSettings::*setter)(bool))
	{
		QCheckBox* box = new QCheckBox(label, this);
		box->setToolTip(tooltip);
		box->setChecked(checked);
		RegisterFocusable(box);
		connect(box, &QCheckBox::toggled, &m_settings, setter);
		return box;
	}

	QComboBox* LauncherMainWindow::CreateProfileCombo(const QStringList& profiles, const QString& currentProfile, void (LauncherSettings::*setter)(const QString&))
	{
		QComboBox* combo = new QComboBox(this);
		combo->addItems(profiles);
		combo->setCurrentText(currentProfile);
		RegisterFocusable(combo);
		connect(combo, &QComboBox::currentTextChanged, &m_settings, setter);
		return combo;
	}

	QComboBox* LauncherMainWindow::CreateProjectCombo()
	{
		QComboBox* combo = new QComboBox(this);
		combo->setObjectName("ProjectCombo");
		combo->setToolTip("Project used by this process.");
		RegisterFocusable(combo);
		m_projectSelectors.push_back(combo);
		connect(combo, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), [combo, this]() {
			const QString projectId = combo->currentData().toString();
			if (!projectId.isEmpty())
			{
				m_projectModel.SelectProject(projectId);
				SetStatusMessage("Project parameter: " + combo->currentText());
			}
		});
		PopulateProjectCombo(*combo);
		return combo;
	}

	QComboBox* LauncherMainWindow::CreateValueCombo(const QVector<QPair<QString, QString>>& options, const QString& currentValue, void (LauncherSettings::*setter)(const QString&))
	{
		QComboBox* combo = new QComboBox(this);
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
		if (operationId == "workspace.generate-solution" || operationId == "toolchain.check" || operationId == "workspace.setup")
		{
			AddNoOptionsMessage(layout, "No parameters");
			return;
		}

		if (operationId == "project.build.editor")
		{
			AddOptionField(layout, "Project", CreateProjectCombo());
			AddOptionField(layout, "Profile", CreateProfileCombo({"DebugEditor", "DevelopmentEditor", "ShippingEditor"}, m_settings.EditorProfile(), &LauncherSettings::SetEditorProfile));
			QVBoxLayout* moreOptionsLayout = AddMoreOptionsSection(layout);
			AddOptionCheckBox(*moreOptionsLayout, CreateBoundCheckBox("Force configure", "Regenerate before building.", m_settings.ForceConfigure(), &LauncherSettings::SetForceConfigure));
			return;
		}

		if (operationId == "project.build.runtime")
		{
			AddOptionField(layout, "Project", CreateProjectCombo());
			AddOptionField(layout, "Profile", CreateProfileCombo({"DebugGame", "DevelopmentGame", "ShippingGame"}, m_settings.RuntimeProfile(), &LauncherSettings::SetRuntimeProfile));
			QVBoxLayout* moreOptionsLayout = AddMoreOptionsSection(layout);
			AddOptionCheckBox(*moreOptionsLayout, CreateBoundCheckBox("Force configure", "Regenerate before building.", m_settings.ForceConfigure(), &LauncherSettings::SetForceConfigure));
			return;
		}

		if (operationId == "cook.shaders")
		{
			AddOptionField(layout, "Project", CreateProjectCombo());
			AddOptionField(layout, "Profile", CreateProfileCombo({"DebugGame", "DevelopmentGame", "ShippingGame"}, m_settings.RuntimeProfile(), &LauncherSettings::SetRuntimeProfile));
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
			QVBoxLayout* moreOptionsLayout = AddMoreOptionsSection(layout);
			QCheckBox* forceRecookBox = CreateBoundCheckBox("Force recook", "Clean and recook instead of incremental cook.", m_settings.ForceRecook(), &LauncherSettings::SetForceRecook);
			forceRecookBox->setObjectName("WarningCheckBox");
			AddOptionCheckBox(*moreOptionsLayout, forceRecookBox);
			QCheckBox* confirmRecookBox = CreateBoundCheckBox("Confirm recook cleanup", "Required before destructive force recook runs.", m_settings.ConfirmForceRecook(), &LauncherSettings::SetConfirmForceRecook);
			confirmRecookBox->setObjectName("DestructiveCheckBox");
			QWidget* confirmRecookRow = AddOptionCheckBox(*moreOptionsLayout, confirmRecookBox);
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
			AddOptionField(layout, "Project", CreateProjectCombo());
			AddOptionField(layout, "Profile", CreateProfileCombo({"DebugGame", "DevelopmentGame", "ShippingGame"}, m_settings.RuntimeProfile(), &LauncherSettings::SetRuntimeProfile));
			QVBoxLayout* moreOptionsLayout = AddMoreOptionsSection(layout);
			QCheckBox* forceRecookBox = CreateBoundCheckBox("Force recook", "Clean and recook instead of incremental cook.", m_settings.ForceRecook(), &LauncherSettings::SetForceRecook);
			forceRecookBox->setObjectName("WarningCheckBox");
			AddOptionCheckBox(*moreOptionsLayout, forceRecookBox);
			QCheckBox* confirmRecookBox = CreateBoundCheckBox("Confirm recook cleanup", "Required before destructive force recook runs.", m_settings.ConfirmForceRecook(), &LauncherSettings::SetConfirmForceRecook);
			confirmRecookBox->setObjectName("DestructiveCheckBox");
			QWidget* confirmRecookRow = AddOptionCheckBox(*moreOptionsLayout, confirmRecookBox);
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

		if (operationId == "project.launch.editor")
		{
			AddOptionField(layout, "Project", CreateProjectCombo());
			AddOptionField(layout, "Profile", CreateProfileCombo({"DebugEditor", "DevelopmentEditor", "ShippingEditor"}, m_settings.EditorProfile(), &LauncherSettings::SetEditorProfile));
			return;
		}

		if (operationId == "project.launch.runtime")
		{
			AddOptionField(layout, "Project", CreateProjectCombo());
			AddOptionField(layout, "Profile", CreateProfileCombo({"DebugGame", "DevelopmentGame", "ShippingGame"}, m_settings.RuntimeProfile(), &LauncherSettings::SetRuntimeProfile));
			return;
		}

		if (operationId == "quality.format")
		{
			QComboBox* formatModeBox = new QComboBox(this);
			RegisterFocusable(formatModeBox);
			formatModeBox->addItem("Check formatting", "check");
			formatModeBox->addItem("Apply formatting", "apply");
			const int formatModeIndex = formatModeBox->findData(m_settings.FormatMode());
			formatModeBox->setCurrentIndex(formatModeIndex >= 0 ? formatModeIndex : 0);
			connect(formatModeBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), [formatModeBox, this]() {
				m_settings.SetFormatMode(formatModeBox->currentData().toString());
			});
			AddOptionField(layout, "Mode", formatModeBox);
			return;
		}

		if (operationId.startsWith("smoke."))
		{
			AddOptionField(layout, "Project", CreateProjectCombo());
			AddOptionField(layout, "Frame limit", CreateValueCombo({{"Default frame limit (120)", ""}, {"60 frames", "60"}, {"120 frames", "120"}, {"300 frames", "300"}, {"600 frames", "600"}}, m_settings.SmokeFrameLimit(), &LauncherSettings::SetSmokeFrameLimit));
			QVBoxLayout* moreOptionsLayout = AddMoreOptionsSection(layout);
			AddOptionField(*moreOptionsLayout, "Backend", CreateValueCombo({{"Default backend", ""}, {"D3D12", "d3d12"}, {"Vulkan", "vulkan"}}, m_settings.SmokeBackend(), &LauncherSettings::SetSmokeBackend));
			AddOptionCheckBox(*moreOptionsLayout, CreateBoundCheckBox("Enable trace", "Capture smoke trace output.", m_settings.SmokeTrace(), &LauncherSettings::SetSmokeTrace));
			AddOptionCheckBox(*moreOptionsLayout, CreateBoundCheckBox("Skip level switching", "Do not switch levels during smoke.", m_settings.SmokeSkipLevelSwitching(), &LauncherSettings::SetSmokeSkipLevelSwitching));
			return;
		}

		if (operationId == "workspace.clean")
		{
			QComboBox* cleanScopeBox = new QComboBox(this);
			RegisterFocusable(cleanScopeBox);
			cleanScopeBox->addItem("Selected Project Cooked Outputs", "selected-cooked");
			cleanScopeBox->addItem("All Cooked Outputs", "all-cooked");
			cleanScopeBox->addItem("Build Tree", "build-tree");
			cleanScopeBox->addItem("Shader Cache", "shader-cache");
			cleanScopeBox->addItem("Third-Party Dependency Cache", "deps");
			cleanScopeBox->addItem("Logs", "logs");
			cleanScopeBox->addItem("Pristine Generated Workspace", "pristine");
			const int cleanScopeIndex = cleanScopeBox->findData(m_settings.CleanScope());
			cleanScopeBox->setCurrentIndex(cleanScopeIndex >= 0 ? cleanScopeIndex : 0);
			AddOptionField(layout, "Scope", cleanScopeBox);
			QWidget* projectRow = AddOptionField(layout, "Project", CreateProjectCombo());
			const auto updateProjectVisibility = [cleanScopeBox, projectRow]() {
				projectRow->setVisible(cleanScopeBox->currentData().toString() == "selected-cooked");
			};
			connect(cleanScopeBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), [cleanScopeBox, updateProjectVisibility, this]() {
				m_settings.SetCleanScope(cleanScopeBox->currentData().toString());
				updateProjectVisibility();
				UpdateRunAvailability();
			});
			updateProjectVisibility();
			QVBoxLayout* moreOptionsLayout = AddMoreOptionsSection(layout);
			QCheckBox* confirmCleanBox = CreateBoundCheckBox("Confirm clean", "Required before destructive clean scopes run.", m_settings.ConfirmClean(), &LauncherSettings::SetConfirmClean);
			confirmCleanBox->setObjectName("DestructiveCheckBox");
			AddOptionCheckBox(*moreOptionsLayout, confirmCleanBox);
			return;
		}

		AddNoOptionsMessage(layout, "No parameters");
	}

	QWidget* LauncherMainWindow::AddOptionField(QVBoxLayout& layout, const QString& label, QWidget* control)
	{
		QFrame* row = new QFrame(this);
		row->setObjectName("OptionRow");
		QHBoxLayout* rowLayout = new QHBoxLayout(row);
		rowLayout->setContentsMargins(0, 0, 0, 0);
		rowLayout->setSpacing(kSpaceMedium + kSpaceTiny);

		QLabel* fieldLabel = CreateFieldLabel(label);
		fieldLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
		fieldLabel->setFixedWidth(kFieldLabelWidth);
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

	QVBoxLayout* LauncherMainWindow::AddMoreOptionsSection(QVBoxLayout& layout)
	{
		QFrame* section = new QFrame(this);
		section->setObjectName("MoreOptionsSection");
		QVBoxLayout* sectionLayout = new QVBoxLayout(section);
		sectionLayout->setContentsMargins(0, kSpaceTiny + kSpaceTiny, 0, 0);
		sectionLayout->setSpacing(kSpaceSmall - kSpaceTiny);

		QPushButton* toggle = new QPushButton("More options", section);
		toggle->setObjectName("MoreOptionsToggle");
		toggle->setCheckable(true);
		toggle->setAccessibleName("Show more options");
		RegisterFocusable(toggle);
		sectionLayout->addWidget(toggle, 0, Qt::AlignLeft);

		QFrame* content = new QFrame(section);
		content->setObjectName("MoreOptionsContent");
		QVBoxLayout* contentLayout = new QVBoxLayout(content);
		contentLayout->setContentsMargins(0, kSpaceTiny, 0, 0);
		contentLayout->setSpacing(kSpaceSmall);
		content->setVisible(false);
		connect(toggle, &QPushButton::toggled, content, &QWidget::setVisible);

		sectionLayout->addWidget(content);
		layout.addWidget(section);
		return contentLayout;
	}

	void LauncherMainWindow::AddNoOptionsMessage(QVBoxLayout& layout, const QString& text)
	{
		QLabel* label = new QLabel(text, this);
		label->setObjectName("MutedLabel");
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
			m_runButton->setEnabled(false);
			m_runButton->setToolTip("Select a workflow before running.");
			return;
		}

		if (OperationNeedsProject(m_selectedOperationId) && m_projectModel.SelectedProjectId().isEmpty())
		{
			m_runButton->setEnabled(false);
			m_runButton->setToolTip("No project discovered. Run Setup Workspace or Check Toolchain, then retry.");
			return;
		}

		const QString title = DisplayNameForOperation(m_selectedOperationId);
		m_runButton->setEnabled(true);
		m_runButton->setToolTip("Start " + title + ". Other running processes keep running.");
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

	QString LauncherMainWindow::DescriptionForOperation(const QString& operationId) const
	{
		const LauncherOperationDescriptor* operation = FindOperationDescriptor(operationId);
		return operation == nullptr ? QString() : operation->Description;
	}

	bool LauncherMainWindow::OperationNeedsProject(const QString& operationId) const
	{
		if (operationId == "workspace.clean")
		{
			return m_settings.CleanScope() == "selected-cooked";
		}

		return operationId.startsWith("project.") || operationId.startsWith("cook.") || operationId.startsWith("smoke.");
	}

	QString LauncherMainWindow::FailureRecoveryHint(const QString& operationId, const QString& statusText) const
	{
		if (OperationNeedsProject(operationId) && m_projectModel.SelectedProjectId().isEmpty())
		{
			return "No project is selected. Run Setup Workspace, then retry this workflow.";
		}
		if (operationId.startsWith("cook.") && m_settings.ForceRecook() && !m_settings.ConfirmForceRecook())
		{
			return "Open More options, enable Confirm recook cleanup, then retry.";
		}
		if (operationId == "workspace.clean" && !m_settings.ConfirmClean())
		{
			return "Open More options, enable Confirm clean, then retry.";
		}

		if (operationId.startsWith("project.build") || statusText.contains("cmake", Qt::CaseInsensitive) || statusText.contains("MSBuild", Qt::CaseInsensitive) || statusText.contains("tool", Qt::CaseInsensitive))
		{
			return "Run Setup > Check Toolchain, then retry this workflow.";
		}

		if (operationId.startsWith("cook."))
		{
			return "Review the output below. If tools or cooked inputs are missing, run Build Cook Tools before retrying.";
		}

		if (operationId.startsWith("smoke.") || operationId.startsWith("project.launch"))
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
		request.SelectedTargets = m_settings.SelectedTargets();
		request.ShaderPackages = m_settings.ShaderPackages();
		request.SmokeBackend = m_settings.SmokeBackend();
		request.SmokeFrameLimit = m_settings.SmokeFrameLimit();
		request.FormatMode = m_settings.FormatMode();
		request.CleanScope = m_settings.CleanScope();
		request.ForceConfigure = m_settings.ForceConfigure();
		request.ForceRecook = m_settings.ForceRecook();
		request.ConfirmForceRecook = m_settings.ConfirmForceRecook();
		request.ConfirmClean = m_settings.ConfirmClean();
		request.SmokeTrace = m_settings.SmokeTrace();
		request.SmokeSkipLevelSwitching = m_settings.SmokeSkipLevelSwitching();
		return request;
	}

	bool LauncherMainWindow::ConfirmRunRequest(const LauncherOperationRequest& request) const
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
			    "Open More options and enable Confirm recook cleanup before running a force recook.");
			return false;
		}
		if (cleanRequested && !request.ConfirmClean)
		{
			QMessageBox::warning(
			    const_cast<LauncherMainWindow*>(this),
			    "Confirmation Required",
			    "Open More options and enable Confirm clean before running this clean workflow.");
			return false;
		}

		const QMessageBox::StandardButton result = QMessageBox::question(
		    const_cast<LauncherMainWindow*>(this),
		    "Confirm Launcher Process",
		    "This process has a confirmed destructive option enabled. Continue?",
		    QMessageBox::Yes | QMessageBox::No,
		    QMessageBox::No);
		return result == QMessageBox::Yes;
	}

	void LauncherMainWindow::SetStatusMessage(const QString& message)
	{
		statusBar()->showMessage(message);
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
		if (m_activeOperationDescription != nullptr)
		{
			m_activeOperationDescription->setText(DescriptionForOperation(operationId));
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
			if (m_workflowGroupButtonGroup != nullptr)
			{
				for (QAbstractButton* button : m_workflowGroupButtonGroup->buttons())
				{
					button->setChecked(button->property("WorkflowIndex").toInt() == workflowIndex);
				}
			}
		}

		UpdateRunAvailability();
		SetStatusMessage("Selected " + title);
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
		const QString title = m_runTitles.value(runId, "Selected process");
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
			m_copyOutputButton->setEnabled(!m_operationOutput->toPlainText().isEmpty());
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
		UpdateRunAvailability();
	}

	void LauncherMainWindow::PopulateProjectCombo(QComboBox& combo) const
	{
		const QSignalBlocker blocker(&combo);
		combo.clear();
		if (m_projectModel.Projects().empty())
		{
			combo.addItem("No projects found", "");
			combo.setToolTip("No projects were discovered in the repository. Run Setup Workspace or inspect project discovery output.");
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
		    {"Setup", "Fresh sync", {"workspace.setup", "toolchain.check", "workspace.generate-solution"}},
		    {"Build", "Compile targets", {"project.build.editor", "project.build.runtime", "cook.tools.prepare"}},
		    {"Cook", "Prepare content", {"cook.project", "cook.shaders", "cook.textures", "cook.assets"}},
		    {"Run", "Launch and verify", {"project.launch.editor", "project.launch.runtime", "smoke.rhi.editor", "smoke.rhi.runtime"}},
		    {"Maintain", "Routine cleanup", {"quality.format", "workspace.clean"}},
		};
	}

	void LauncherMainWindow::ApplyVisualStyle()
	{
		const QString background = "#1f242b";
		const QString shell = "#1b2027";
		const QString panel = "#242a32";
		const QString panelHover = "#2b323b";
		const QString field = "#1f252d";
		const QString border = "#343b45";
		const QString borderStrong = "#454c56";
		const QString divider = "#11161c";
		const QString focus = "#c9d1d9";
		const QString primary = "#0969da";
		const QString primaryHover = "#1f7eed";
		const QString selection = "#0d419d";
		const QString warning = QString::fromLatin1(kColorStateWarning);
		const QString destructive = QString::fromLatin1(kColorStateDestructive);
		const QString textPrimary = "#f0f3f6";
		const QString textBody = "#dce3ec";
		const QString textSecondary = "#9aa4af";
		const QString textMuted = "#8b949e";

		QString style;
		const auto addRule = [&style](const QString& selector, const QString& body) {
			style += selector + " { " + body + " }";
		};

		addRule("QMainWindow, QWidget", "background: " + background + "; color: " + textBody + "; font-family: 'Segoe UI'; font-size: 10pt;");
		addRule("QLabel", "color: " + textBody + "; background: transparent;");
		addRule("#WorkflowSurface", "background: " + background + ";");
		addRule("#ProcessPanel", "background: " + shell + "; border-right: 1px solid " + divider + "; padding: 0;");
		addRule("#OptionsPanel", "background: " + panel + "; border: 1px solid " + border + "; border-radius: 6px;");
		addRule("#OutputPanel", "background: " + shell + "; border-top: 1px solid " + divider + ";");
		addRule("#OptionsScrollArea, #OptionsStack, #OptionsContent, #OperationStack, #MoreOptionsSection, #MoreOptionsContent, #ActivityDetailsPanel", "background: transparent; border: none;");
		addRule("#OptionsScrollArea QWidget", "background: transparent;");
		addRule("#OptionRow", "background: transparent; min-height: 36px;");

		addRule("#ActiveOperationLabel", "color: " + textPrimary + "; font-size: 15pt; font-weight: 700;");
		addRule("#OperationDescription", "color: " + textMuted + "; line-height: 130%;");
		addRule("#WorkflowRailTitle", "color: " + textPrimary + "; font-size: 11pt; font-weight: 700; padding: 0 0 2px 0;");
		addRule("#SectionLabel", "color: " + textPrimary + "; font-size: 10.5pt; font-weight: 700; padding-top: 2px;");
		addRule("#FieldLabel", "color: " + textSecondary + "; font-size: 9pt; font-weight: 600; padding-top: 0;");
		addRule("#MutedLabel", "color: " + textMuted + "; padding: 6px 0;");
		addRule("#ProgressLabel", "color: " + textPrimary + "; font-size: 10.5pt; font-weight: 700;");
		addRule("#ActivitySummary", "color: " + textSecondary + "; background: transparent; font-size: 9pt; font-weight: 600; padding: 0 0 2px 4px;");

		addRule("#WorkflowGroupButton", "background: transparent; color: " + textMuted + "; border: 1px solid transparent; border-radius: 4px; padding: 7px 9px; text-align: left; font-size: 9pt; font-weight: 650; min-width: 78px;");
		addRule("#WorkflowGroupButton:hover", "background: " + panel + "; color: " + textBody + ";");
		addRule("#WorkflowGroupButton:checked", "background: " + field + "; color: " + textPrimary + "; border: 1px solid " + borderStrong + ";");
		addRule("#WorkflowGroupButton:focus", "border: 1px solid " + focus + "; color: " + textPrimary + ";");
		addRule("#WorkflowButton", "background: transparent; color: " + textBody + "; border: 1px solid transparent; border-radius: 4px; padding: 8px 10px; text-align: left; font-size: 10pt; font-weight: 600;");
		addRule("#WorkflowButton:hover", "background: " + panelHover + "; border: 1px solid " + border + ";");
		addRule("#WorkflowButton:checked", "background: " + selection + "; border: 1px solid " + primary + "; color: #ffffff;");
		addRule("#WorkflowButton:focus", "border: 1px solid " + focus + "; color: " + textPrimary + ";");

		addRule("QPushButton", "background: " + primary + "; color: #ffffff; border: none; border-radius: 4px; padding: 8px 14px; font-weight: 650;");
		addRule("QPushButton:hover", "background: " + primaryHover + ";");
		addRule("QPushButton:focus", "border: 1px solid " + focus + ";");
		addRule("QPushButton:disabled", "background: " + border + "; color: " + textMuted + ";");
		addRule("#PrimaryActionButton", "background: " + primary + "; min-width: 96px;");
		addRule("#PrimaryActionButton:hover", "background: " + primaryHover + ";");
		addRule("#SecondaryButton", "background: " + border + "; color: " + textBody + "; border: 1px solid " + borderStrong + ";");
		addRule("#SecondaryButton:hover", "background: " + panelHover + ";");
		addRule("#SecondaryButton:focus", "border: 1px solid " + focus + "; color: " + textPrimary + ";");
		addRule("#MoreOptionsToggle", "background: transparent; color: " + textSecondary + "; border: 1px solid " + border + "; border-radius: 4px; padding: 5px 10px; font-size: 9pt; font-weight: 650;");
		addRule("#MoreOptionsToggle:hover", "background: " + panelHover + "; color: " + textBody + "; border: 1px solid " + borderStrong + ";");
		addRule("#MoreOptionsToggle:checked", "background: " + border + "; color: " + textPrimary + "; border: 1px solid " + borderStrong + ";");
		addRule("#MoreOptionsToggle:focus", "border: 1px solid " + focus + "; color: " + textPrimary + ";");

		addRule("QComboBox, QTextEdit", "background: " + field + "; border: 1px solid " + borderStrong + "; border-radius: 4px; padding: 7px 9px; color: " + textBody + "; selection-background-color: " + selection + ";");
		addRule("QComboBox:focus, QTextEdit:focus", "border: 1px solid " + focus + ";");
		addRule("QCheckBox", "spacing: 8px; padding: 3px 0; color: " + textBody + ";");
		addRule("QCheckBox:focus", "border: 1px solid " + focus + "; border-radius: 4px; color: " + textPrimary + ";");
		addRule("#WarningCheckBox", "color: " + warning + ";");
		addRule("#DestructiveCheckBox", "color: " + destructive + ";");

		addRule("QListWidget", "background: transparent; border: none; border-radius: 0; padding: 0; outline: 0;");
		addRule("QListWidget:focus", "border: 1px solid " + focus + ";");
		addRule("QListWidget::item", "padding: 9px 11px; border-radius: 4px; color: " + textBody + ";");
		addRule("QListWidget::item:selected", "background: " + selection + "; color: #ffffff;");
		addRule("#ActivityList", "background: transparent; border: none; border-right: 1px solid " + border + "; border-radius: 0; padding: 0 12px 0 0;");
		addRule("#OperationOutput", "background: transparent; border: none; border-radius: 0; padding: 4px 0 0 4px; font-family: 'Cascadia Mono'; font-size: 9pt;");
		addRule("QProgressBar", "background: " + border + "; border: none; border-radius: 3px; color: " + textBody + "; text-align: center; min-height: 6px; max-height: 6px;");
		addRule("QProgressBar::chunk", "background: " + borderStrong + "; border-radius: 3px;");
		addRule("QStatusBar", "background: " + shell + "; color: " + textMuted + "; border-top: 1px solid " + divider + ";");

		setStyleSheet(style);
	}
}