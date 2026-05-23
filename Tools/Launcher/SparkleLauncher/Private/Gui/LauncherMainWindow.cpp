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
		resize(1240, 800);
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
		layout->setContentsMargins(32, 30, 32, 24);
		layout->setSpacing(22);
		layout->addWidget(CreatePageTitle("Workflows", "Pick a workflow, configure its parameters, then run and monitor it below.", surface));
		layout->addWidget(CreateProcessPicker(surface));
		layout->addWidget(CreateOptionsPanel(surface), 1);
		return surface;
	}

	QWidget* LauncherMainWindow::CreateProcessPicker(QWidget* parent)
	{
		QFrame* panel = new QFrame(parent);
		panel->setObjectName("ProcessPanel");
		QVBoxLayout* layout = new QVBoxLayout(panel);
		layout->setContentsMargins(0, 0, 0, 0);
		layout->setSpacing(12);
		layout->addWidget(CreateSectionLabel("1  Choose workflow"));

		m_processButtonGroup = new QButtonGroup(this);
		m_processButtonGroup->setExclusive(true);

		m_categoryTabs = new QTabWidget(panel);
		m_categoryTabs->setObjectName("ProcessTabs");
		m_categoryTabs->setDocumentMode(true);

		for (const WorkflowDefinition& workflow : CreateWorkflowDefinitions())
		{
			QWidget* tabPage = new QWidget(m_categoryTabs);
			QGridLayout* actionLayout = new QGridLayout();
			actionLayout->setContentsMargins(0, 14, 0, 0);
			actionLayout->setHorizontalSpacing(12);
			actionLayout->setVerticalSpacing(10);
			for (int index = 0; index < workflow.OperationIds.size(); ++index)
			{
				const QString& operationId = workflow.OperationIds[index];
				QPushButton* button = CreateProcessButton(DisplayNameForOperation(operationId), operationId, tabPage);
				m_processButtonGroup->addButton(button);
				actionLayout->addWidget(button, index / 2, index % 2);
			}
			actionLayout->setColumnStretch(0, 1);
			actionLayout->setColumnStretch(1, 1);
			tabPage->setLayout(actionLayout);
			m_categoryTabs->addTab(tabPage, workflow.Title);
		}
		connect(m_processButtonGroup, &QButtonGroup::buttonClicked, this, &LauncherMainWindow::SelectProcessButton);
		layout->addWidget(m_categoryTabs);
		return panel;
	}

	QPushButton* LauncherMainWindow::CreateProcessButton(const QString& label, const QString& operationId, QWidget* parent)
	{
		QPushButton* button = new QPushButton(label, parent);
		button->setObjectName(operationId == "workspace.generate-solution" ? "PrimaryWorkflowButton" : "WorkflowButton");
		button->setCheckable(true);
		button->setMinimumHeight(46);
		button->setProperty("OperationId", operationId);
		button->setToolTip(DisplayNameForOperation(operationId));
		return button;
	}

	QWidget* LauncherMainWindow::CreateOptionsPanel(QWidget* parent)
	{
		QFrame* panel = new QFrame(parent);
		panel->setObjectName("OptionsPanel");
		QVBoxLayout* layout = new QVBoxLayout(panel);
		layout->setContentsMargins(22, 20, 22, 18);
		layout->setSpacing(14);

		QLabel* stepLabel = CreateSectionLabel("2  Configure and run");
		layout->addWidget(stepLabel);

		m_activeOperationLabel = new QLabel("Selected: Generate Solution", panel);
		m_activeOperationLabel->setObjectName("ActiveOperationLabel");
		layout->addWidget(m_activeOperationLabel);

		m_activeOperationDescription = new QLabel(DescriptionForOperation("workspace.generate-solution"), panel);
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

		QHBoxLayout* actionLayout = new QHBoxLayout();
		actionLayout->setSpacing(10);
		actionLayout->addStretch(1);
		m_previewButton = new QPushButton("Preview", panel);
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
		layout->setContentsMargins(0, 4, 8, 0);
		layout->setSpacing(12);
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
		layout->setContentsMargins(22, 14, 22, 16);
		layout->setSpacing(10);

		QLabel* monitorLabel = CreateSectionLabel("3  Monitor progress");
		layout->addWidget(monitorLabel);

		QHBoxLayout* progressLayout = new QHBoxLayout();
		progressLayout->setSpacing(14);
		m_progressLabel = new QLabel("No processes running", panel);
		m_progressLabel->setObjectName("ProgressLabel");
		progressLayout->addWidget(m_progressLabel);
		m_progressBar = new QProgressBar(panel);
		m_progressBar->setObjectName("ProgressBar");
		m_progressBar->setTextVisible(true);
		progressLayout->addWidget(m_progressBar, 1);
		layout->addLayout(progressLayout);

		QHBoxLayout* activityHeaderLayout = new QHBoxLayout();
		activityHeaderLayout->setSpacing(14);
		QLabel* activityHeader = CreateFieldLabel("Activity");
		activityHeader->setMinimumWidth(280);
		activityHeaderLayout->addWidget(activityHeader, 1);
		activityHeaderLayout->addWidget(CreateFieldLabel("Output"), 3);
		layout->addLayout(activityHeaderLayout);

		QHBoxLayout* activityLayout = new QHBoxLayout();
		activityLayout->setSpacing(14);
		m_activityList = new QListWidget(panel);
		m_activityList->setObjectName("ActivityList");
		m_activityList->setMinimumWidth(280);
		m_activityList->setMaximumHeight(146);
		connect(m_activityList, &QListWidget::currentItemChanged, this, &LauncherMainWindow::DisplaySelectedRunOutput);
		activityLayout->addWidget(m_activityList, 1);

		m_operationOutput = new QTextEdit(panel);
		m_operationOutput->setObjectName("OperationOutput");
		m_operationOutput->setReadOnly(true);
		m_operationOutput->setMinimumHeight(120);
		m_operationOutput->setMaximumHeight(168);
		m_operationOutput->setToolTip("Select an activity to view its output.");
		m_operationOutput->setPlainText("Preview a workflow to inspect planned work, or run it to stream output here.");
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

	QLabel* LauncherMainWindow::CreateFieldLabel(const QString& title) const
	{
		QLabel* label = new QLabel(title);
		label->setObjectName("FieldLabel");
		return label;
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

	QComboBox* LauncherMainWindow::CreateValueCombo(const QVector<QPair<QString, QString>>& options, const QString& currentValue, void (LauncherSettings::*setter)(const QString&))
	{
		QComboBox* combo = new QComboBox(this);
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
			AddNoOptionsMessage(layout, "No parameters. This workflow uses the repository and all discovered projects.");
			return;
		}

		if (operationId == "project.build.editor")
		{
			layout.addWidget(CreateSectionLabel("Editor build"));
			layout.addWidget(CreateFieldLabel("Project"));
			layout.addWidget(CreateProjectCombo());
			layout.addWidget(CreateFieldLabel("Profile"));
			layout.addWidget(CreateProfileCombo({"DebugEditor", "DevelopmentEditor", "ShippingEditor"}, m_settings.EditorProfile(), &LauncherSettings::SetEditorProfile));
			layout.addWidget(CreateBoundCheckBox("Force configure", "Regenerate before building.", &LauncherSettings::SetForceConfigure));
			return;
		}

		if (operationId == "project.build.runtime")
		{
			layout.addWidget(CreateSectionLabel("Runtime build"));
			layout.addWidget(CreateFieldLabel("Project"));
			layout.addWidget(CreateProjectCombo());
			layout.addWidget(CreateFieldLabel("Profile"));
			layout.addWidget(CreateProfileCombo({"DebugGame", "DevelopmentGame", "ShippingGame"}, m_settings.RuntimeProfile(), &LauncherSettings::SetRuntimeProfile));
			layout.addWidget(CreateBoundCheckBox("Force configure", "Regenerate before building.", &LauncherSettings::SetForceConfigure));
			return;
		}

		if (operationId == "cook.shaders")
		{
			layout.addWidget(CreateSectionLabel("Cook shaders"));
			layout.addWidget(CreateFieldLabel("Project"));
			layout.addWidget(CreateProjectCombo());
			layout.addWidget(CreateFieldLabel("Profile"));
			layout.addWidget(CreateProfileCombo({"DebugGame", "DevelopmentGame", "ShippingGame"}, m_settings.RuntimeProfile(), &LauncherSettings::SetRuntimeProfile));
			layout.addWidget(CreateFieldLabel("Shader package"));
			layout.addWidget(CreateValueCombo(
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
			layout.addWidget(CreateBoundCheckBox("Force recook", "Clean and recook instead of incremental cook.", &LauncherSettings::SetForceRecook));
			layout.addWidget(CreateBoundCheckBox("Confirm recook cleanup", "Required before destructive force recook runs.", &LauncherSettings::SetConfirmForceRecook));
			return;
		}

		if (operationId.startsWith("cook."))
		{
			layout.addWidget(CreateSectionLabel("Cook"));
			layout.addWidget(CreateFieldLabel("Project"));
			layout.addWidget(CreateProjectCombo());
			layout.addWidget(CreateFieldLabel("Profile"));
			layout.addWidget(CreateProfileCombo({"DebugGame", "DevelopmentGame", "ShippingGame"}, m_settings.RuntimeProfile(), &LauncherSettings::SetRuntimeProfile));
			layout.addWidget(CreateBoundCheckBox("Force recook", "Clean and recook instead of incremental cook.", &LauncherSettings::SetForceRecook));
			layout.addWidget(CreateBoundCheckBox("Confirm recook cleanup", "Required before destructive force recook runs.", &LauncherSettings::SetConfirmForceRecook));
			return;
		}

		if (operationId == "project.launch.editor")
		{
			layout.addWidget(CreateSectionLabel("Launch editor"));
			layout.addWidget(CreateFieldLabel("Project"));
			layout.addWidget(CreateProjectCombo());
			layout.addWidget(CreateFieldLabel("Profile"));
			layout.addWidget(CreateProfileCombo({"DebugEditor", "DevelopmentEditor", "ShippingEditor"}, m_settings.EditorProfile(), &LauncherSettings::SetEditorProfile));
			return;
		}

		if (operationId == "project.launch.runtime")
		{
			layout.addWidget(CreateSectionLabel("Launch runtime"));
			layout.addWidget(CreateFieldLabel("Project"));
			layout.addWidget(CreateProjectCombo());
			layout.addWidget(CreateFieldLabel("Profile"));
			layout.addWidget(CreateProfileCombo({"DebugGame", "DevelopmentGame", "ShippingGame"}, m_settings.RuntimeProfile(), &LauncherSettings::SetRuntimeProfile));
			return;
		}

		if (operationId == "quality.format")
		{
			layout.addWidget(CreateSectionLabel("Format"));
			layout.addWidget(CreateFieldLabel("Mode"));
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
			layout.addWidget(CreateFieldLabel("Project"));
			layout.addWidget(CreateProjectCombo());
			layout.addWidget(CreateFieldLabel("Backend"));
			layout.addWidget(CreateValueCombo({{"Default backend", ""}, {"D3D12", "d3d12"}, {"Vulkan", "vulkan"}}, m_settings.SmokeBackend(), &LauncherSettings::SetSmokeBackend));
			layout.addWidget(CreateFieldLabel("Frame limit"));
			layout.addWidget(CreateValueCombo({{"Default frame limit (120)", ""}, {"60 frames", "60"}, {"120 frames", "120"}, {"300 frames", "300"}, {"600 frames", "600"}}, m_settings.SmokeFrameLimit(), &LauncherSettings::SetSmokeFrameLimit));
			layout.addWidget(CreateBoundCheckBox("Enable trace", "Capture smoke trace output.", &LauncherSettings::SetSmokeTrace));
			layout.addWidget(CreateBoundCheckBox("Skip level switching", "Do not switch levels during smoke.", &LauncherSettings::SetSmokeSkipLevelSwitching));
			return;
		}

		if (operationId == "workspace.clean")
		{
			layout.addWidget(CreateSectionLabel("Clean"));
			layout.addWidget(CreateFieldLabel("Project"));
			layout.addWidget(CreateProjectCombo());
			layout.addWidget(CreateFieldLabel("Scope"));
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

	QString LauncherMainWindow::DescriptionForOperation(const QString& operationId) const
	{
		const LauncherOperationDescriptor* operation = FindOperationDescriptor(operationId);
		return operation == nullptr ? QString() : operation->Description;
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
			m_activeOperationLabel->setText(title);
		}
		if (m_activeOperationDescription != nullptr)
		{
			m_activeOperationDescription->setText(DescriptionForOperation(operationId));
		}
		if (m_runButton != nullptr)
		{
			m_runButton->setText("Run");
			m_runButton->setToolTip("Start " + title + ". Other running processes keep running.");
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

		if (m_categoryTabs != nullptr)
		{
			const QVector<WorkflowDefinition> workflows = CreateWorkflowDefinitions();
			for (int index = 0; index < workflows.size(); ++index)
			{
				if (workflows[index].OperationIds.contains(operationId))
				{
					m_categoryTabs->setCurrentIndex(index);
					break;
				}
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
		    "QMainWindow, QWidget { background: #11151a; color: #e8edf2; font-family: 'Segoe UI'; font-size: 10pt; }"
		    "QLabel { color: #c7d0de; background: transparent; }"
		    "#WorkflowSurface { background: #11151a; }"
		    "#OutputPanel { background: #0f1217; border-top: 1px solid #252c36; }"
		    "#ActiveOperationLabel { color: #ffffff; font-size: 14pt; font-weight: 700; }"
		    "#OperationDescription { color: #98a6b7; line-height: 130%; }"
		    "#ProgressLabel { color: #ffffff; font-size: 10.5pt; font-weight: 700; }"
		    "#ProcessPanel { background: transparent; border: none; }"
		    "#OptionsPanel { background: #151a21; border-top: 1px solid #2a3340; }"
		    "#OptionsScrollArea, #OptionsStack { background: transparent; }"
		    "QStatusBar { background: #0f1217; color: #8f9bae; border-top: 1px solid #252c36; }"
		    "QListWidget { background: transparent; border: none; border-radius: 0; padding: 0; outline: 0; }"
		    "QListWidget::item { padding: 10px 12px; border-radius: 4px; color: #bac6d6; }"
		    "QListWidget::item:selected { background: #2f6fed; color: #ffffff; }"
		    "QTabWidget#ProcessTabs::pane { border: none; background: transparent; margin-top: 8px; }"
		    "QTabWidget#ProcessTabs QWidget { background: transparent; }"
		    "QTabBar::tab { background: transparent; color: #9aa8ba; border: none; border-bottom: 2px solid transparent; padding: 8px 15px 10px 15px; font-weight: 650; }"
		    "QTabBar::tab:selected { color: #ffffff; border-bottom: 2px solid #2fc49a; }"
		    "QTabBar::tab:hover { color: #dce5f2; }"
		    "#WorkflowButton, #PrimaryWorkflowButton { color: #dce5f2; border: 1px solid #2a3441; border-radius: 4px; padding: 10px 13px; text-align: left; font-size: 10pt; font-weight: 650; }"
		    "#WorkflowButton { background: #171c23; }"
		    "#WorkflowButton:hover { background: #1d2530; border: 1px solid #445367; }"
		    "#WorkflowButton:checked { background: #1d2c3f; border: 1px solid #6b96ff; color: #ffffff; }"
		    "#PrimaryWorkflowButton { background: #1b6b56; border: 1px solid #2aa883; color: #ffffff; }"
		    "#PrimaryWorkflowButton:hover { background: #217c64; }"
		    "#PrimaryWorkflowButton:checked { background: #249576; border: 1px solid #7ae1c4; }"
		    "#PageTitle h1 { color: #ffffff; font-size: 18pt; margin: 0; }"
		    "#PageTitle p { color: #98a6b7; margin-top: 5px; }"
		    "#SectionLabel { color: #f4f7fb; font-size: 10.5pt; font-weight: 700; padding-top: 2px; }"
		    "#FieldLabel { color: #8d9bad; font-size: 8.5pt; font-weight: 700; text-transform: uppercase; padding-top: 4px; }"
		    "#MutedLabel { color: #98a6b7; padding: 8px 0; }"
		    "QPushButton { background: #2f6fed; color: #ffffff; border: none; border-radius: 4px; padding: 9px 15px; font-weight: 650; }"
		    "QPushButton:hover { background: #3d7bff; }"
		    "#SecondaryButton { background: #222a35; color: #d8e0ea; }"
		    "#SecondaryButton:hover { background: #2c3542; }"
		    "#PrimaryActionButton { background: #21a67f; min-width: 96px; }"
		    "#PrimaryActionButton:hover { background: #28bb91; }"
		    "QProgressBar { background: #1a2029; border: none; border-radius: 3px; color: #d8e0ea; text-align: center; min-height: 14px; }"
		    "QProgressBar::chunk { background: #2fc49a; border-radius: 3px; }"
		    "QComboBox, QTextEdit { background: #171d25; border: 1px solid #2c3542; border-radius: 4px; padding: 8px; color: #e8edf2; selection-background-color: #2f6fed; }"
		    "QComboBox:focus, QTextEdit:focus { border: 1px solid #5f8cff; }"
		    "#ActivityList { background: transparent; border: none; border-right: 1px solid #252c36; border-radius: 0; padding: 0 12px 0 0; }"
		    "#OperationOutput { background: transparent; border: none; border-radius: 0; padding: 4px 0 0 4px; font-family: 'Cascadia Mono'; font-size: 9pt; }"
		    "QCheckBox { spacing: 8px; padding: 4px 0; color: #d2dbe8; }");
	}
}