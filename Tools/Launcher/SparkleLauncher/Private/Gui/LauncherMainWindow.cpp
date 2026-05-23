#include "LauncherMainWindow.h"

#include "LauncherBackend.h"
#include "LauncherProjectModel.h"
#include "LauncherSettings.h"

#include <QtCore/QSignalBlocker>
#include <QtCore/Qt>
#include <QtGui/QTextCursor>
#include <QtGui/QTextDocument>
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
		rootLayout->addWidget(CreateAppHeader());

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

	QWidget* LauncherMainWindow::CreateAppHeader()
	{
		QFrame* header = new QFrame(this);
		header->setObjectName("AppHeader");
		QHBoxLayout* layout = new QHBoxLayout(header);
		layout->setContentsMargins(18, 10, 18, 10);
		layout->setSpacing(24);

		QLabel* repositoryLabel = new QLabel("Current repository\n<b>" + QString::fromStdString(m_repositoryRoot.filename().string()) + "</b>", header);
		repositoryLabel->setObjectName("HeaderContextLabel");
		repositoryLabel->setTextFormat(Qt::RichText);
		layout->addWidget(repositoryLabel);

		QLabel* modeLabel = new QLabel("Launcher mode\n<b>Local workflows</b>", header);
		modeLabel->setObjectName("HeaderContextLabel");
		modeLabel->setTextFormat(Qt::RichText);
		layout->addWidget(modeLabel);

		layout->addStretch(1);
		QLabel* hintLabel = new QLabel("Select a workflow to configure and run", header);
		hintLabel->setObjectName("HeaderHintLabel");
		layout->addWidget(hintLabel);
		return header;
	}

	QWidget* LauncherMainWindow::CreateWorkflowSurface()
	{
		QFrame* surface = new QFrame(this);
		surface->setObjectName("WorkflowSurface");
		QHBoxLayout* layout = new QHBoxLayout(surface);
		layout->setContentsMargins(18, 18, 18, 18);
		layout->setSpacing(18);
		layout->addWidget(CreateProcessPicker(surface), 0);
		layout->addWidget(CreateOptionsPanel(surface), 1);
		return surface;
	}

	QWidget* LauncherMainWindow::CreateProcessPicker(QWidget* parent)
	{
		QFrame* panel = new QFrame(parent);
		panel->setObjectName("ProcessPanel");
		panel->setFixedWidth(356);
		QVBoxLayout* layout = new QVBoxLayout(panel);
		layout->setContentsMargins(0, 0, 0, 0);
		layout->setSpacing(10);
		layout->addWidget(CreatePageTitle("Workflows", "Choose one action to configure.", panel));

		m_processButtonGroup = new QButtonGroup(this);
		m_processButtonGroup->setExclusive(true);

		m_categoryTabs = new QTabWidget(panel);
		m_categoryTabs->setObjectName("ProcessTabs");
		m_categoryTabs->setDocumentMode(true);

		for (const WorkflowDefinition& workflow : CreateWorkflowDefinitions())
		{
			QWidget* tabPage = new QWidget(m_categoryTabs);
			QVBoxLayout* actionLayout = new QVBoxLayout();
			actionLayout->setContentsMargins(0, 8, 0, 0);
			actionLayout->setSpacing(2);
			for (int index = 0; index < workflow.OperationIds.size(); ++index)
			{
				const QString& operationId = workflow.OperationIds[index];
				QPushButton* button = CreateProcessButton(DisplayNameForOperation(operationId), operationId, tabPage);
				m_processButtonGroup->addButton(button);
				actionLayout->addWidget(button);
			}
			actionLayout->addStretch(1);
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
		button->setObjectName("WorkflowButton");
		button->setCheckable(true);
		button->setMinimumHeight(32);
		button->setProperty("OperationId", operationId);
		button->setToolTip(DescriptionForOperation(operationId));
		return button;
	}

	QWidget* LauncherMainWindow::CreateOptionsPanel(QWidget* parent)
	{
		QFrame* panel = new QFrame(parent);
		panel->setObjectName("OptionsPanel");
		QVBoxLayout* layout = new QVBoxLayout(panel);
		layout->setContentsMargins(22, 18, 22, 18);
		layout->setSpacing(10);

		m_activeOperationLabel = new QLabel("No workflow selected", panel);
		m_activeOperationLabel->setObjectName("ActiveOperationLabel");
		layout->addWidget(m_activeOperationLabel);

		m_activeOperationDescription = new QLabel("Choose a workflow from the left. Its parameters and run controls will appear here.", panel);
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
		actionLayout->setSpacing(10);
		actionLayout->addStretch(1);
		m_runButton = new QPushButton(style()->standardIcon(QStyle::SP_MediaPlay), "Run", panel);
		m_runButton->setObjectName("PrimaryActionButton");
		m_runButton->setToolTip("Start this process. Other running processes keep running.");
		m_runButton->setEnabled(false);
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
		layout->setContentsMargins(0, 8, 8, 0);
		layout->setSpacing(8);
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

		QLabel* monitorLabel = CreateSectionLabel("Activity");
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
		m_operationOutput->setPlainText("Run a workflow to stream output here.");
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
			AddNoOptionsMessage(layout, "No parameters");
			return;
		}

		if (operationId == "project.build.editor")
		{
			AddOptionField(layout, "Project", CreateProjectCombo());
			AddOptionField(layout, "Profile", CreateProfileCombo({"DebugEditor", "DevelopmentEditor", "ShippingEditor"}, m_settings.EditorProfile(), &LauncherSettings::SetEditorProfile));
			AddOptionCheckBox(layout, CreateBoundCheckBox("Force configure", "Regenerate before building.", &LauncherSettings::SetForceConfigure));
			return;
		}

		if (operationId == "project.build.runtime")
		{
			AddOptionField(layout, "Project", CreateProjectCombo());
			AddOptionField(layout, "Profile", CreateProfileCombo({"DebugGame", "DevelopmentGame", "ShippingGame"}, m_settings.RuntimeProfile(), &LauncherSettings::SetRuntimeProfile));
			AddOptionCheckBox(layout, CreateBoundCheckBox("Force configure", "Regenerate before building.", &LauncherSettings::SetForceConfigure));
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
			AddOptionCheckBox(layout, CreateBoundCheckBox("Force recook", "Clean and recook instead of incremental cook.", &LauncherSettings::SetForceRecook));
			AddOptionCheckBox(layout, CreateBoundCheckBox("Confirm recook cleanup", "Required before destructive force recook runs.", &LauncherSettings::SetConfirmForceRecook));
			return;
		}

		if (operationId.startsWith("cook."))
		{
			AddOptionField(layout, "Project", CreateProjectCombo());
			AddOptionField(layout, "Profile", CreateProfileCombo({"DebugGame", "DevelopmentGame", "ShippingGame"}, m_settings.RuntimeProfile(), &LauncherSettings::SetRuntimeProfile));
			AddOptionCheckBox(layout, CreateBoundCheckBox("Force recook", "Clean and recook instead of incremental cook.", &LauncherSettings::SetForceRecook));
			AddOptionCheckBox(layout, CreateBoundCheckBox("Confirm recook cleanup", "Required before destructive force recook runs.", &LauncherSettings::SetConfirmForceRecook));
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
			formatModeBox->addItem("Check formatting", "check");
			formatModeBox->addItem("Apply formatting", "apply");
			connect(formatModeBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), [formatModeBox, this]() {
				m_settings.SetFormatMode(formatModeBox->currentData().toString());
			});
			AddOptionField(layout, "Mode", formatModeBox);
			return;
		}

		if (operationId.startsWith("smoke."))
		{
			AddOptionField(layout, "Project", CreateProjectCombo());
			AddOptionField(layout, "Backend", CreateValueCombo({{"Default backend", ""}, {"D3D12", "d3d12"}, {"Vulkan", "vulkan"}}, m_settings.SmokeBackend(), &LauncherSettings::SetSmokeBackend));
			AddOptionField(layout, "Frame limit", CreateValueCombo({{"Default frame limit (120)", ""}, {"60 frames", "60"}, {"120 frames", "120"}, {"300 frames", "300"}, {"600 frames", "600"}}, m_settings.SmokeFrameLimit(), &LauncherSettings::SetSmokeFrameLimit));
			AddOptionCheckBox(layout, CreateBoundCheckBox("Enable trace", "Capture smoke trace output.", &LauncherSettings::SetSmokeTrace));
			AddOptionCheckBox(layout, CreateBoundCheckBox("Skip level switching", "Do not switch levels during smoke.", &LauncherSettings::SetSmokeSkipLevelSwitching));
			return;
		}

		if (operationId == "workspace.clean")
		{
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
			AddOptionField(layout, "Project", CreateProjectCombo());
			AddOptionField(layout, "Scope", cleanScopeBox);
			AddOptionCheckBox(layout, CreateBoundCheckBox("Confirm clean", "Required before destructive clean scopes run.", &LauncherSettings::SetConfirmClean));
			return;
		}

		AddNoOptionsMessage(layout, "No parameters");
	}

	void LauncherMainWindow::AddOptionField(QVBoxLayout& layout, const QString& label, QWidget* control)
	{
		QFrame* row = new QFrame(this);
		row->setObjectName("OptionRow");
		QHBoxLayout* rowLayout = new QHBoxLayout(row);
		rowLayout->setContentsMargins(0, 0, 0, 0);
		rowLayout->setSpacing(14);

		QLabel* fieldLabel = CreateFieldLabel(label);
		fieldLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
		fieldLabel->setFixedWidth(116);
		rowLayout->addWidget(fieldLabel);
		rowLayout->addWidget(control, 1);
		layout.addWidget(row);
	}

	void LauncherMainWindow::AddOptionCheckBox(QVBoxLayout& layout, QCheckBox* checkBox)
	{
		QFrame* row = new QFrame(this);
		row->setObjectName("OptionRow");
		QHBoxLayout* rowLayout = new QHBoxLayout(row);
		rowLayout->setContentsMargins(0, 0, 0, 0);
		rowLayout->setSpacing(14);
		rowLayout->addSpacing(130);
		rowLayout->addWidget(checkBox, 1);
		layout.addWidget(row);
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
		    "QMainWindow, QWidget { background: #1f242b; color: #dce3ec; font-family: 'Segoe UI'; font-size: 10pt; }"
		    "QLabel { color: #c9d1d9; background: transparent; }"
		    "#AppHeader { background: #1b2027; border-bottom: 1px solid #0f1318; }"
		    "#HeaderContextLabel { color: #8b949e; font-size: 9pt; }"
		    "#HeaderContextLabel b { color: #f0f3f6; font-weight: 700; }"
		    "#HeaderHintLabel { color: #8b949e; }"
		    "#WorkflowSurface { background: #1f242b; }"
		    "#OutputPanel { background: #1b2027; border-top: 1px solid #11161c; }"
		    "#ActiveOperationLabel { color: #f0f3f6; font-size: 13pt; font-weight: 700; }"
		    "#OperationDescription { color: #8b949e; line-height: 130%; }"
		    "#ProgressLabel { color: #ffffff; font-size: 10.5pt; font-weight: 700; }"
		    "#ProcessPanel { background: #1b2027; border-right: 1px solid #11161c; padding: 0; }"
		    "#OptionsPanel { background: #242a32; border: 1px solid #343b45; border-radius: 6px; }"
		    "#OptionsScrollArea, #OptionsStack, #OptionsContent { background: transparent; border: none; }"
		    "#OptionsScrollArea QWidget { background: transparent; }"
		    "#OptionRow { background: transparent; min-height: 36px; }"
		    "QStatusBar { background: #1b2027; color: #8b949e; border-top: 1px solid #11161c; }"
		    "QListWidget { background: transparent; border: none; border-radius: 0; padding: 0; outline: 0; }"
		    "QListWidget::item { padding: 10px 12px; border-radius: 4px; color: #c9d1d9; }"
		    "QListWidget::item:selected { background: #0969da; color: #ffffff; }"
		    "QTabWidget#ProcessTabs::pane { border: none; background: transparent; margin-top: 8px; }"
		    "QTabWidget#ProcessTabs QWidget { background: transparent; }"
		    "QTabBar::tab { background: transparent; color: #8b949e; border: none; border-bottom: 2px solid transparent; padding: 8px 12px 9px 12px; font-weight: 650; }"
		    "QTabBar::tab:selected { color: #f0f3f6; border-bottom: 2px solid #0969da; }"
		    "QTabBar::tab:hover { color: #dce3ec; }"
		    "#WorkflowButton { color: #dce3ec; border: 1px solid transparent; border-radius: 4px; padding: 8px 10px; text-align: left; font-size: 10pt; font-weight: 600; background: transparent; }"
		    "#WorkflowButton:hover { background: #252b33; border: 1px solid #343b45; }"
		    "#WorkflowButton:checked { background: #0d419d; border: 1px solid #0969da; color: #ffffff; }"
		    "#PageTitle h1 { color: #f0f3f6; font-size: 13pt; margin: 0; }"
		    "#PageTitle p { color: #8b949e; margin-top: 4px; }"
		    "#SectionLabel { color: #f0f3f6; font-size: 10.5pt; font-weight: 700; padding-top: 2px; }"
		    "#FieldLabel { color: #9aa4af; font-size: 9pt; font-weight: 600; padding-top: 0; }"
		    "#MutedLabel { color: #8b949e; padding: 6px 0; }"
		    "QPushButton { background: #0969da; color: #ffffff; border: none; border-radius: 4px; padding: 8px 14px; font-weight: 650; }"
		    "QPushButton:hover { background: #1f7eed; }"
		    "QPushButton:disabled { background: #343b45; color: #8b949e; }"
		    "#SecondaryButton { background: #30363d; color: #dce3ec; border: 1px solid #454c56; }"
		    "#SecondaryButton:hover { background: #373e47; }"
		    "#PrimaryActionButton { background: #0969da; min-width: 96px; }"
		    "#PrimaryActionButton:hover { background: #1f7eed; }"
		    "QProgressBar { background: #30363d; border: none; border-radius: 3px; color: #dce3ec; text-align: center; min-height: 14px; }"
		    "QProgressBar::chunk { background: #0969da; border-radius: 3px; }"
		    "QComboBox, QTextEdit { background: #1b2027; border: 1px solid #454c56; border-radius: 4px; padding: 7px 9px; color: #dce3ec; selection-background-color: #0969da; }"
		    "QComboBox:focus, QTextEdit:focus { border: 1px solid #0969da; }"
		    "#ActivityList { background: transparent; border: none; border-right: 1px solid #30363d; border-radius: 0; padding: 0 12px 0 0; }"
		    "#OperationOutput { background: transparent; border: none; border-radius: 0; padding: 4px 0 0 4px; font-family: 'Cascadia Mono'; font-size: 9pt; }"
		    "QCheckBox { spacing: 8px; padding: 3px 0; color: #dce3ec; }");
	}
}