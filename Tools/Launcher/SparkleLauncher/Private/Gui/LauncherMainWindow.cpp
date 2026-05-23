#include "LauncherMainWindow.h"

#include "LauncherBackend.h"
#include "LauncherProjectModel.h"
#include "LauncherSettings.h"

#include <QtCore/QSignalBlocker>
#include <QtCore/Qt>
#include <QtGui/QTextCursor>
#include <QtGui/QTextDocument>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QScrollArea>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QStyle>
#include <QtWidgets/QWidget>

#include <utility>

namespace SparkleLauncher
{
	static constexpr int kMaxOperationOutputCharacters = 1000000;

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
		resize(1220, 760);
		statusBar()->showMessage("Ready");

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

		ApplyVisualStyle();

		connect(&m_projectModel, &LauncherProjectModel::ProjectsChanged, this, &LauncherMainWindow::PopulateProjectSelectors);
		connect(&m_projectModel, &LauncherProjectModel::SelectionChanged, this, &LauncherMainWindow::PopulateProjectSelectors);
		connect(&m_projectModel, &LauncherProjectModel::ProjectDiscoveryFailed, this, &LauncherMainWindow::SetStartupNotice);
		connect(&m_backend, &LauncherBackend::OperationPreviewReady, this, &LauncherMainWindow::DisplayOperationPreview);
		connect(&m_backend, &LauncherBackend::OperationPreviewFailed, this, &LauncherMainWindow::DisplayOperationPreviewError);
		connect(&m_backend, &LauncherBackend::OperationStarted, this, &LauncherMainWindow::DisplayOperationStarted);
		connect(&m_backend, &LauncherBackend::OperationOutputReceived, this, &LauncherMainWindow::AppendOperationOutput);
		connect(&m_backend, &LauncherBackend::OperationFinished, this, &LauncherMainWindow::DisplayOperationFinished);

		SetSelectedOperation("workspace.generate-solution");
		UpdateProgress();
		RefreshProjects();
	}

	void LauncherMainWindow::SetStartupNotice(const QString& message)
	{
		if (!message.isEmpty())
		{
			SetStatusMessage(message);
		}
	}

	void LauncherMainWindow::RefreshProjects()
	{
		m_projectModel.Refresh(m_repositoryRoot);
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

	void LauncherMainWindow::PreviewSelectedOperation()
	{
		if (m_selectedOperationId.isEmpty())
		{
			m_operationOutput->setPlainText("Select a process before opening details.");
			SetStatusMessage("No process selected");
			return;
		}

		m_activeRunId.clear();
		SetStatusMessage("Loading details for " + DisplayNameForOperation(m_selectedOperationId));
		m_backend.RequestOperationPreview(BuildOperationRequest(m_selectedOperationId));
	}

	void LauncherMainWindow::RunSelectedOperation()
	{
		if (m_selectedOperationId.isEmpty())
		{
			m_operationOutput->setPlainText("Select a process before running.");
			SetStatusMessage("No process selected");
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

	void LauncherMainWindow::DisplayOperationPreview(const QString&, const QString& title, const QString& previewText, bool canRun)
	{
		m_operationOutput->setPlainText(title + QString(canRun ? " is ready.\n\n" : " is blocked.\n\n") + previewText);
		SetStatusMessage(title + QString(canRun ? " ready" : " blocked"));
	}

	void LauncherMainWindow::DisplayOperationPreviewError(const QString&, const QString& message)
	{
		m_operationOutput->setPlainText(message);
		SetStatusMessage(message);
	}

	void LauncherMainWindow::DisplayOperationStarted(const QString& runId, const QString&, const QString& title)
	{
		QListWidgetItem* item = m_runItems.value(runId, nullptr);
		if (item != nullptr)
		{
			item->setText("Running: " + title);
		}

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

	void LauncherMainWindow::DisplayOperationFinished(const QString& runId, const QString&, const QString& title, const QString& statusText, int exitCode)
	{
		QListWidgetItem* item = m_runItems.value(runId, nullptr);
		if (item != nullptr)
		{
			item->setText(QString(exitCode == 0 ? "Done: " : "Needs attention: ") + title);
		}

		AppendRunOutput(runId, "\n" + title + " finished: " + statusText + "\n");
		++m_finishedRunCount;
		ShowRunOutput(runId);
		SetStatusMessage(title + " finished: " + statusText);
		UpdateProgress();
	}

	QWidget* LauncherMainWindow::CreateWorkflowSurface()
	{
		QFrame* surface = new QFrame(this);
		surface->setObjectName("WorkflowSurface");
		QVBoxLayout* layout = new QVBoxLayout(surface);
		layout->setContentsMargins(24, 24, 24, 18);
		layout->setSpacing(14);
		layout->addWidget(CreatePageTitle("Process", "Choose a category, review its options, then run it.", surface));

		QHBoxLayout* contentLayout = new QHBoxLayout();
		contentLayout->setSpacing(16);
		contentLayout->addWidget(CreateProcessPicker(surface), 5);
		contentLayout->addWidget(CreateOptionsPanel(surface), 4);
		layout->addLayout(contentLayout, 1);
		return surface;
	}

	QWidget* LauncherMainWindow::CreateProcessPicker(QWidget* parent)
	{
		QFrame* panel = new QFrame(parent);
		panel->setObjectName("ProcessPanel");
		QVBoxLayout* layout = new QVBoxLayout(panel);
		layout->setContentsMargins(16, 16, 16, 16);
		layout->setSpacing(12);
		layout->addWidget(CreateSectionLabel("Pick a process"));

		m_processButtonGroup = new QButtonGroup(this);
		m_processButtonGroup->setExclusive(true);

		QScrollArea* scrollArea = new QScrollArea(panel);
		scrollArea->setWidgetResizable(true);
		scrollArea->setFrameShape(QFrame::NoFrame);
		scrollArea->setObjectName("ProcessScrollArea");
		QWidget* content = new QWidget(scrollArea);
		QVBoxLayout* processLayout = new QVBoxLayout(content);
		processLayout->setContentsMargins(0, 0, 8, 0);
		processLayout->setSpacing(14);

		for (const WorkflowDefinition& workflow : CreateWorkflowDefinitions())
		{
			processLayout->addWidget(CreateSectionLabel(workflow.Title));
			QGridLayout* actionLayout = new QGridLayout();
			actionLayout->setHorizontalSpacing(10);
			actionLayout->setVerticalSpacing(8);
			for (int index = 0; index < workflow.OperationIds.size(); ++index)
			{
				const QString& operationId = workflow.OperationIds[index];
				QPushButton* button = CreateProcessButton(DisplayNameForOperation(operationId), operationId, content);
				m_processButtonGroup->addButton(button);
				actionLayout->addWidget(button, index / 2, index % 2);
			}
			processLayout->addLayout(actionLayout);
		}
		processLayout->addStretch(1);
		scrollArea->setWidget(content);
		connect(m_processButtonGroup, &QButtonGroup::buttonClicked, this, &LauncherMainWindow::SelectProcessButton);
		layout->addWidget(scrollArea, 1);
		return panel;
	}

	QPushButton* LauncherMainWindow::CreateProcessButton(const QString& label, const QString& operationId, QWidget* parent)
	{
		QPushButton* button = new QPushButton(label, parent);
		button->setObjectName(operationId == "workspace.generate-solution" ? "PrimaryWorkflowButton" : "WorkflowButton");
		button->setCheckable(true);
		button->setMinimumHeight(48);
		button->setProperty("OperationId", operationId);
		button->setToolTip(DisplayNameForOperation(operationId));
		return button;
	}

	QWidget* LauncherMainWindow::CreateOptionsPanel(QWidget* parent)
	{
		QFrame* panel = new QFrame(parent);
		panel->setObjectName("OptionsPanel");
		QVBoxLayout* layout = new QVBoxLayout(panel);
		layout->setContentsMargins(16, 16, 16, 16);
		layout->setSpacing(12);

		m_activeOperationLabel = new QLabel("Selected: Generate Solution", panel);
		m_activeOperationLabel->setObjectName("ActiveOperationLabel");
		layout->addWidget(m_activeOperationLabel);

		m_optionsStack = new QStackedWidget(panel);
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

		QHBoxLayout* actionLayout = new QHBoxLayout();
		actionLayout->setSpacing(10);
		actionLayout->addStretch(1);
		m_previewButton = new QPushButton("Details", panel);
		m_previewButton->setObjectName("SecondaryButton");
		m_previewButton->setToolTip("Preview readiness and planned work without running.");
		connect(m_previewButton, &QPushButton::clicked, this, &LauncherMainWindow::PreviewSelectedOperation);
		actionLayout->addWidget(m_previewButton);
		m_runButton = new QPushButton(style()->standardIcon(QStyle::SP_MediaPlay), "Run", panel);
		m_runButton->setObjectName("PrimaryActionButton");
		m_runButton->setToolTip("Start this process. Other running processes keep running.");
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
		QVBoxLayout* layout = new QVBoxLayout(content);
		layout->setContentsMargins(0, 0, 8, 0);
		layout->setSpacing(10);
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
		layout->setContentsMargins(18, 12, 18, 14);
		layout->setSpacing(8);

		QHBoxLayout* progressLayout = new QHBoxLayout();
		progressLayout->setSpacing(10);
		m_progressLabel = new QLabel("No processes running", panel);
		m_progressLabel->setObjectName("ProgressLabel");
		progressLayout->addWidget(m_progressLabel);
		m_progressBar = new QProgressBar(panel);
		m_progressBar->setObjectName("ProgressBar");
		m_progressBar->setTextVisible(true);
		progressLayout->addWidget(m_progressBar, 1);
		layout->addLayout(progressLayout);

		QHBoxLayout* activityLayout = new QHBoxLayout();
		activityLayout->setSpacing(10);
		m_activityList = new QListWidget(panel);
		m_activityList->setObjectName("ActivityList");
		m_activityList->setMinimumWidth(260);
		m_activityList->setMaximumHeight(132);
		connect(m_activityList, &QListWidget::currentItemChanged, this, &LauncherMainWindow::DisplaySelectedRunOutput);
		activityLayout->addWidget(m_activityList, 1);

		m_operationOutput = new QTextEdit(panel);
		m_operationOutput->setObjectName("OperationOutput");
		m_operationOutput->setReadOnly(true);
		m_operationOutput->setMinimumHeight(108);
		m_operationOutput->setMaximumHeight(154);
		m_operationOutput->setToolTip("Select an activity to view its output.");
		m_operationOutput->setPlainText("Select a process, adjust its options, then click Run.");
		activityLayout->addWidget(m_operationOutput, 3);
		layout->addLayout(activityLayout);
		return panel;
	}

	QLabel* LauncherMainWindow::CreatePageTitle(const QString& title, const QString& subtitle, QWidget* parent) const
	{
		const QString subtitleMarkup = subtitle.isEmpty() ? QString() : "<p>" + subtitle + "</p>";
		QLabel* label = new QLabel("<h1>" + title + "</h1>" + subtitleMarkup, parent);
		label->setObjectName("PageTitle");
		label->setTextFormat(Qt::RichText);
		return label;
	}

	QLabel* LauncherMainWindow::CreateSectionLabel(const QString& title) const
	{
		QLabel* label = new QLabel(title);
		label->setObjectName("SectionLabel");
		return label;
	}

	QLineEdit* LauncherMainWindow::CreateBoundLineEdit(const QString& placeholder, const QString& tooltip, void (LauncherSettings::*setter)(const QString&))
	{
		QLineEdit* edit = new QLineEdit(this);
		edit->setPlaceholderText(placeholder);
		edit->setToolTip(tooltip);
		connect(edit, &QLineEdit::textChanged, &m_settings, setter);
		return edit;
	}

	QCheckBox* LauncherMainWindow::CreateBoundCheckBox(const QString& label, const QString& tooltip, void (LauncherSettings::*setter)(bool))
	{
		QCheckBox* box = new QCheckBox(label, this);
		box->setToolTip(tooltip);
		connect(box, &QCheckBox::toggled, &m_settings, setter);
		return box;
	}

	QComboBox* LauncherMainWindow::CreateProfileCombo(const QStringList& profiles, const QString& currentProfile, void (LauncherSettings::*setter)(const QString&))
	{
		QComboBox* combo = new QComboBox(this);
		combo->addItems(profiles);
		combo->setCurrentText(currentProfile);
		connect(combo, &QComboBox::currentTextChanged, &m_settings, setter);
		return combo;
	}

	QComboBox* LauncherMainWindow::CreateProjectCombo()
	{
		QComboBox* combo = new QComboBox(this);
		combo->setObjectName("ProjectCombo");
		combo->setToolTip("Project used by this process.");
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

	void LauncherMainWindow::AddOptionsForOperation(QVBoxLayout& layout, const QString& operationId)
	{
		if (operationId == "workspace.generate-solution" || operationId == "toolchain.check" || operationId == "workspace.setup")
		{
			AddNoOptionsMessage(layout, "Workspace process. Applies to the repository and all discovered projects.");
			return;
		}

		if (operationId == "project.build.editor")
		{
			layout.addWidget(CreateSectionLabel("Editor build"));
			layout.addWidget(CreateProjectCombo());
			layout.addWidget(CreateProfileCombo({"DebugEditor", "DevelopmentEditor", "ShippingEditor"}, m_settings.EditorProfile(), &LauncherSettings::SetEditorProfile));
			layout.addWidget(CreateBoundLineEdit("Optional build targets", "Comma-separated target filter.", &LauncherSettings::SetSelectedTargets));
			layout.addWidget(CreateBoundCheckBox("Force configure", "Regenerate before building.", &LauncherSettings::SetForceConfigure));
			return;
		}

		if (operationId == "project.build.runtime")
		{
			layout.addWidget(CreateSectionLabel("Runtime build"));
			layout.addWidget(CreateProjectCombo());
			layout.addWidget(CreateProfileCombo({"DebugGame", "DevelopmentGame", "ShippingGame"}, m_settings.RuntimeProfile(), &LauncherSettings::SetRuntimeProfile));
			layout.addWidget(CreateBoundLineEdit("Optional build targets", "Comma-separated target filter.", &LauncherSettings::SetSelectedTargets));
			layout.addWidget(CreateBoundCheckBox("Force configure", "Regenerate before building.", &LauncherSettings::SetForceConfigure));
			return;
		}

		if (operationId.startsWith("cook."))
		{
			layout.addWidget(CreateSectionLabel("Cook"));
			layout.addWidget(CreateProjectCombo());
			layout.addWidget(CreateProfileCombo({"DebugGame", "DevelopmentGame", "ShippingGame"}, m_settings.RuntimeProfile(), &LauncherSettings::SetRuntimeProfile));
			layout.addWidget(CreateBoundLineEdit("Shader packages", "Optional shader package ids.", &LauncherSettings::SetShaderPackages));
			layout.addWidget(CreateBoundCheckBox("Force recook", "Clean and recook instead of incremental cook.", &LauncherSettings::SetForceRecook));
			layout.addWidget(CreateBoundCheckBox("Confirm recook cleanup", "Required before destructive force recook runs.", &LauncherSettings::SetConfirmForceRecook));
			return;
		}

		if (operationId == "project.launch.editor")
		{
			layout.addWidget(CreateSectionLabel("Launch editor"));
			layout.addWidget(CreateProjectCombo());
			layout.addWidget(CreateProfileCombo({"DebugEditor", "DevelopmentEditor", "ShippingEditor"}, m_settings.EditorProfile(), &LauncherSettings::SetEditorProfile));
			return;
		}

		if (operationId == "project.launch.runtime")
		{
			layout.addWidget(CreateSectionLabel("Launch runtime"));
			layout.addWidget(CreateProjectCombo());
			layout.addWidget(CreateProfileCombo({"DebugGame", "DevelopmentGame", "ShippingGame"}, m_settings.RuntimeProfile(), &LauncherSettings::SetRuntimeProfile));
			return;
		}

		if (operationId == "quality.format")
		{
			layout.addWidget(CreateSectionLabel("Format"));
			QComboBox* formatModeBox = new QComboBox(this);
			formatModeBox->addItem("Check formatting", "check");
			formatModeBox->addItem("Apply formatting", "apply");
			connect(formatModeBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), [formatModeBox, this]() {
				m_settings.SetFormatMode(formatModeBox->currentData().toString());
			});
			layout.addWidget(formatModeBox);
			return;
		}

		if (operationId.startsWith("smoke."))
		{
			layout.addWidget(CreateSectionLabel("Smoke test"));
			layout.addWidget(CreateProjectCombo());
			layout.addWidget(CreateBoundLineEdit("Backend", "Optional smoke backend.", &LauncherSettings::SetSmokeBackend));
			layout.addWidget(CreateBoundLineEdit("Frame limit", "Optional smoke frame limit.", &LauncherSettings::SetSmokeFrameLimit));
			layout.addWidget(CreateBoundCheckBox("Enable trace", "Capture smoke trace output.", &LauncherSettings::SetSmokeTrace));
			layout.addWidget(CreateBoundCheckBox("Skip level switching", "Do not switch levels during smoke.", &LauncherSettings::SetSmokeSkipLevelSwitching));
			return;
		}

		if (operationId == "workspace.clean")
		{
			layout.addWidget(CreateSectionLabel("Clean"));
			layout.addWidget(CreateProjectCombo());
			QComboBox* cleanScopeBox = new QComboBox(this);
			cleanScopeBox->addItem("Selected Project Cooked Outputs", "selected-cooked");
			cleanScopeBox->addItem("All Cooked Outputs", "all-cooked");
			cleanScopeBox->addItem("Build Tree", "build-tree");
			cleanScopeBox->addItem("Shader Cache", "shader-cache");
			cleanScopeBox->addItem("Third-Party Dependency Cache", "deps");
			cleanScopeBox->addItem("Logs", "logs");
			cleanScopeBox->addItem("Pristine Generated Workspace", "pristine");
			connect(cleanScopeBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), [cleanScopeBox, this]() {
				m_settings.SetCleanScope(cleanScopeBox->currentData().toString());
			});
			layout.addWidget(cleanScopeBox);
			layout.addWidget(CreateBoundCheckBox("Confirm clean", "Required before destructive clean scopes run.", &LauncherSettings::SetConfirmClean));
			return;
		}

		AddNoOptionsMessage(layout, "No options needed for this process.");
	}

	void LauncherMainWindow::AddNoOptionsMessage(QVBoxLayout& layout, const QString& text)
	{
		QLabel* label = new QLabel(text, this);
		label->setObjectName("MutedLabel");
		label->setWordWrap(true);
		layout.addWidget(label);
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
		if (!request.ForceRecook && !request.ConfirmClean)
		{
			return true;
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
		if (m_activeOperationLabel != nullptr)
		{
			m_activeOperationLabel->setText("Selected: " + title);
		}
		if (m_runButton != nullptr)
		{
			m_runButton->setText("Run " + title);
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

		SetStatusMessage("Selected " + title);
	}

	void LauncherMainWindow::RegisterRun(const QString& runId, const QString& title)
	{
		++m_startedRunCount;
		QListWidgetItem* item = new QListWidgetItem("Queued: " + title, m_activityList);
		item->setData(Qt::UserRole, runId);
		m_runItems.insert(runId, item);
		m_runOutputs.insert(runId, title + " queued.\n");
		m_activityList->setCurrentItem(item);
		m_activeRunId = runId;
		UpdateProgress();
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
		m_operationOutput->setPlainText(m_runOutputs.value(runId));
		m_operationOutput->moveCursor(QTextCursor::End);
	}

	void LauncherMainWindow::UpdateProgress()
	{
		if (m_progressBar == nullptr || m_progressLabel == nullptr)
		{
			return;
		}

		const int runningCount = m_startedRunCount - m_finishedRunCount;
		if (m_startedRunCount == 0)
		{
			m_progressBar->setRange(0, 1);
			m_progressBar->setValue(0);
			m_progressBar->setFormat("0/0 complete");
			m_progressLabel->setText("No processes running");
			return;
		}

		m_progressBar->setRange(0, m_startedRunCount);
		m_progressBar->setValue(m_finishedRunCount);
		m_progressBar->setFormat("%v/%m complete");
		m_progressLabel->setText(QStringLiteral("%1 running, %2 complete").arg(runningCount).arg(m_finishedRunCount));
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
			combo.addItem("No projects discovered", "");
			combo.setEnabled(false);
			return;
		}

		combo.setEnabled(true);
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
		    {"Workspace", "Setup / Clean", {"workspace.generate-solution", "workspace.setup", "toolchain.check", "workspace.clean"}},
		    {"Build", "Editor / Runtime", {"project.build.editor", "project.build.runtime"}},
		    {"Cook", "Assets / Shaders", {"cook.tools.prepare", "cook.project", "cook.shaders", "cook.textures", "cook.assets"}},
		    {"Launch", "Editor / Runtime", {"project.launch.editor", "project.launch.runtime"}},
		    {"Utilities", "Format / Smoke", {"quality.format", "smoke.rhi.editor", "smoke.rhi.runtime"}},
		};
	}

	void LauncherMainWindow::ApplyVisualStyle()
	{
		setStyleSheet(
		    "QMainWindow, QWidget { background: #15171c; color: #e8edf2; font-family: 'Segoe UI'; font-size: 10pt; }"
		    "#WorkflowSurface { background: #15171c; }"
		    "#OutputPanel { background: #101217; border-top: 1px solid #2b3038; }"
		    "#ActiveOperationLabel, #ProgressLabel { color: #ffffff; font-size: 11pt; font-weight: 700; }"
		    "#ProcessPanel, #OptionsPanel { background: transparent; border: none; }"
		    "#OptionsScrollArea, #ProcessScrollArea { background: transparent; }"
		    "QStatusBar { background: #101217; color: #8f9bae; border-top: 1px solid #2b3038; }"
		    "QListWidget { background: #1d222b; border: 1px solid #303743; border-radius: 6px; padding: 6px; outline: 0; }"
		    "QListWidget::item { padding: 9px; border-radius: 4px; }"
		    "QListWidget::item:selected { background: #2f6fed; color: #ffffff; }"
		    "#WorkflowButton, #PrimaryWorkflowButton { color: #dce5f2; border: 1px solid #2f3744; border-radius: 4px; padding: 10px 12px; text-align: left; font-size: 10.5pt; font-weight: 650; }"
		    "#WorkflowButton { background: #1a1f28; }"
		    "#WorkflowButton:hover { background: #222936; border: 1px solid #566277; }"
		    "#WorkflowButton:checked { background: #24324a; border: 1px solid #5f8cff; color: #ffffff; }"
		    "#PrimaryWorkflowButton { background: #1d6f5c; border: 1px solid #27876f; color: #ffffff; }"
		    "#PrimaryWorkflowButton:hover { background: #238369; }"
		    "#PrimaryWorkflowButton:checked { background: #249576; border: 1px solid #6bd4b8; }"
		    "#PageTitle h1 { color: #ffffff; font-size: 18pt; margin: 0; }"
		    "#PageTitle p { color: #9da9bb; margin-top: 4px; }"
		    "#SectionLabel { color: #ffffff; font-size: 11pt; font-weight: 700; padding-top: 4px; }"
		    "#MutedLabel { color: #9da9bb; }"
		    "QPushButton { background: #2f6fed; color: #ffffff; border: none; border-radius: 5px; padding: 9px 14px; font-weight: 600; }"
		    "QPushButton:hover { background: #3d7bff; }"
		    "#SecondaryButton { background: #262d38; color: #d8e0ea; }"
		    "#SecondaryButton:hover { background: #303846; }"
		    "#PrimaryActionButton { background: #1f9d78; }"
		    "#PrimaryActionButton:hover { background: #26b98e; }"
		    "QProgressBar { background: #1d222b; border: 1px solid #303743; border-radius: 6px; color: #d8e0ea; text-align: center; min-height: 18px; }"
		    "QProgressBar::chunk { background: #1f9d78; border-radius: 5px; }"
		    "QComboBox, QLineEdit, QTextEdit { background: #1d222b; border: 1px solid #303743; border-radius: 6px; padding: 8px; color: #e8edf2; }"
		    "QComboBox:focus, QLineEdit:focus, QTextEdit:focus { border: 1px solid #5f8cff; }"
		    "QCheckBox { spacing: 8px; padding: 4px; }"
		    "QLabel { color: #c7d0de; }");
	}
}