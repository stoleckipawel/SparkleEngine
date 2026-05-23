#include "LauncherMainWindow.h"

#include "LauncherBackend.h"
#include "LauncherProjectModel.h"
#include "LauncherSettings.h"

#include <QtCore/Qt>
#include <QtGui/QTextCursor>
#include <QtGui/QTextDocument>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QRadioButton>
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
		setMinimumSize(1060, 660);
		resize(1220, 740);
		statusBar()->showMessage("Ready");

		QWidget* centralWidget = new QWidget(this);
		QVBoxLayout* rootLayout = new QVBoxLayout(centralWidget);
		rootLayout->setContentsMargins(0, 0, 0, 0);
		rootLayout->setSpacing(0);
		rootLayout->addWidget(CreateHeader());

		QHBoxLayout* bodyLayout = new QHBoxLayout();
		bodyLayout->setContentsMargins(0, 0, 0, 0);
		bodyLayout->setSpacing(0);
		bodyLayout->addWidget(CreateProjectRail());
		bodyLayout->addWidget(CreateWorkflowSurface(), 1);
		bodyLayout->addWidget(CreateAdvancedDrawer());
		rootLayout->addLayout(bodyLayout, 1);
		rootLayout->addWidget(CreateOutputPanel());
		setCentralWidget(centralWidget);

		ApplyVisualStyle();

		connect(&m_projectModel, &LauncherProjectModel::ProjectsChanged, this, &LauncherMainWindow::PopulateProjects);
		connect(&m_projectModel, &LauncherProjectModel::ProjectDiscoveryFailed, this, &LauncherMainWindow::SetStartupNotice);
		connect(&m_backend, &LauncherBackend::OperationPreviewReady, this, &LauncherMainWindow::DisplayOperationPreview);
		connect(&m_backend, &LauncherBackend::OperationPreviewFailed, this, &LauncherMainWindow::DisplayOperationPreviewError);
		connect(&m_backend, &LauncherBackend::OperationStarted, this, &LauncherMainWindow::DisplayOperationStarted);
		connect(&m_backend, &LauncherBackend::OperationOutputReceived, this, &LauncherMainWindow::AppendOperationOutput);
		connect(&m_backend, &LauncherBackend::OperationFinished, this, &LauncherMainWindow::DisplayOperationFinished);

		SetSelectedOperation("project.build.editor");
		SetStatusMessage("Ready");
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

	void LauncherMainWindow::SelectProjectFromList()
	{
		QListWidgetItem* selectedItem = m_projectList == nullptr ? nullptr : m_projectList->currentItem();
		if (selectedItem == nullptr || selectedItem->data(Qt::UserRole).toString().isEmpty())
		{
			return;
		}

		m_projectModel.SelectProject(selectedItem->data(Qt::UserRole).toString());
		SetStatusMessage("Project selected: " + selectedItem->text());
	}

	void LauncherMainWindow::SelectWorkflow(int index)
	{
		if (m_workflowStack != nullptr && index >= 0 && index < m_workflowStack->count())
		{
			m_workflowStack->setCurrentIndex(index);
		}

		const QVector<WorkflowDefinition> workflows = CreateWorkflowDefinitions();
		if (index >= 0 && index < workflows.size() && !workflows[index].OperationIds.isEmpty())
		{
			SetSelectedOperation(workflows[index].OperationIds.front());
			SetStatusMessage(workflows[index].Title + " workflow selected");
		}
	}

	void LauncherMainWindow::SelectOperationButton(QAbstractButton* button)
	{
		if (button == nullptr)
		{
			return;
		}

		SetSelectedOperation(button->property("OperationId").toString());
		PreviewSelectedOperation();
	}

	void LauncherMainWindow::ToggleAdvancedDrawer()
	{
		if (m_advancedDrawer == nullptr || m_advancedButton == nullptr)
		{
			return;
		}

		const bool visible = !m_advancedDrawer->isVisible();
		m_advancedDrawer->setVisible(visible);
		m_advancedButton->setText(visible ? "Options ^" : "Options v");
		SetStatusMessage(visible ? "Options shown" : "Options hidden");
	}

	void LauncherMainWindow::PreviewSelectedOperation()
	{
		if (m_operationInProgress)
		{
			SetStatusMessage("Operation is already running");
			return;
		}

		if (m_selectedOperationId.isEmpty())
		{
			m_operationOutput->setPlainText("Choose a workflow action to preview what the launcher will do.");
			SetStatusMessage("No operation selected");
			return;
		}

		SetStatusMessage("Previewing " + DisplayNameForOperation(m_selectedOperationId));
		m_backend.RequestOperationPreview(BuildOperationRequest(m_selectedOperationId));
	}

	void LauncherMainWindow::RunSelectedOperation()
	{
		if (m_operationInProgress)
		{
			SetStatusMessage("Operation is already running");
			return;
		}

		if (m_selectedOperationId.isEmpty())
		{
			m_operationOutput->setPlainText("Choose a workflow action before running a launcher operation.");
			SetStatusMessage("No operation selected");
			return;
		}

		LauncherOperationRequest request = BuildOperationRequest(m_selectedOperationId);
		if (!ConfirmRunRequest(request))
		{
			SetStatusMessage("Operation canceled");
			return;
		}

		SetStatusMessage("Running " + DisplayNameForOperation(m_selectedOperationId));
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

	void LauncherMainWindow::DisplayOperationStarted(const QString&, const QString& title)
	{
		m_operationInProgress = true;
		SetOperationControlsEnabled(false);
		m_operationOutput->setPlainText(title + "\n\nOperation started. Output will stream here.\n");
		SetStatusMessage(title + " running");
	}

	void LauncherMainWindow::AppendOperationOutput(const QString&, const QString& outputText)
	{
		m_operationOutput->moveCursor(QTextCursor::End);
		m_operationOutput->insertPlainText(outputText);
		const int overflowCharacters = m_operationOutput->document()->characterCount() - kMaxOperationOutputCharacters;
		if (overflowCharacters > 0)
		{
			QTextCursor trimCursor(m_operationOutput->document());
			trimCursor.setPosition(0);
			trimCursor.setPosition(overflowCharacters, QTextCursor::KeepAnchor);
			trimCursor.removeSelectedText();
		}
		m_operationOutput->moveCursor(QTextCursor::End);
	}

	void LauncherMainWindow::DisplayOperationFinished(const QString&, const QString& title, const QString& statusText, int)
	{
		m_operationOutput->append("\n" + title + " finished: " + statusText);
		m_operationInProgress = false;
		SetOperationControlsEnabled(true);
		SetStatusMessage(title + " finished: " + statusText);
	}

	QWidget* LauncherMainWindow::CreateHeader()
	{
		QFrame* header = new QFrame(this);
		header->setObjectName("Header");
		QHBoxLayout* layout = new QHBoxLayout(header);
		layout->setContentsMargins(18, 14, 18, 12);
		layout->setSpacing(10);

		QLabel* title = new QLabel("Sparkle Launcher", header);
		title->setObjectName("ProductLabel");
		layout->addWidget(title);
		layout->addStretch(1);

		QPushButton* refreshButton = new QPushButton(style()->standardIcon(QStyle::SP_BrowserReload), "Refresh", header);
		refreshButton->setToolTip("Rescan repository projects.");
		connect(refreshButton, &QPushButton::clicked, this, &LauncherMainWindow::RefreshProjects);
		layout->addWidget(refreshButton);

		m_advancedButton = new QPushButton("Options v", header);
		m_advancedButton->setObjectName("SecondaryButton");
		m_advancedButton->setToolTip("Show profile and rarely used operation options.");
		connect(m_advancedButton, &QPushButton::clicked, this, &LauncherMainWindow::ToggleAdvancedDrawer);
		layout->addWidget(m_advancedButton);
		return header;
	}

	QWidget* LauncherMainWindow::CreateProjectRail()
	{
		QFrame* rail = new QFrame(this);
		rail->setObjectName("ProjectRail");
		rail->setFixedWidth(220);
		QVBoxLayout* layout = new QVBoxLayout(rail);
		layout->setContentsMargins(14, 18, 14, 14);
		layout->setSpacing(10);
		layout->addWidget(CreatePageTitle("Project", "", rail));

		m_projectList = new QListWidget(rail);
		m_projectList->setObjectName("ProjectList");
		m_projectList->setToolTip("Select the project used by build, cook, launch, validation, and workspace workflows.");
		connect(m_projectList, &QListWidget::currentItemChanged, this, &LauncherMainWindow::SelectProjectFromList);
		layout->addWidget(m_projectList, 1);

		m_statusLabel = new QLabel("Ready", rail);
		m_statusLabel->setObjectName("StatusLabel");
		m_statusLabel->setWordWrap(true);
		layout->addWidget(m_statusLabel);
		return rail;
	}

	QWidget* LauncherMainWindow::CreateWorkflowSurface()
	{
		QFrame* surface = new QFrame(this);
		surface->setObjectName("WorkflowSurface");
		QVBoxLayout* layout = new QVBoxLayout(surface);
		layout->setContentsMargins(28, 28, 28, 24);
		layout->setSpacing(16);
		layout->addWidget(CreatePageTitle("Run a workflow", "Pick a project, then click the thing you want done.", surface));

		QGridLayout* actionLayout = new QGridLayout();
		actionLayout->setSpacing(14);
		const QVector<QPair<QString, QString>> primaryActions = {
		    {"Generate Solution", "workspace.generate-solution"},
		    {"Build Editor", "project.build.editor"},
		    {"Cook Project", "cook.project"},
		    {"Launch Editor", "project.launch.editor"},
		    {"Validate", "quality.validate"},
		};
		for (int index = 0; index < primaryActions.size(); ++index)
		{
			const QString operationId = primaryActions[index].second;
			QPushButton* button = new QPushButton(primaryActions[index].first, surface);
			button->setObjectName(index == 0 ? "PrimaryWorkflowButton" : "WorkflowButton");
			button->setMinimumHeight(108);
			button->setToolTip(DisplayNameForOperation(operationId));
			connect(button, &QPushButton::clicked, [this, operationId]() {
				SetSelectedOperation(operationId);
				RunSelectedOperation();
			});
			actionLayout->addWidget(button, index / 2, index % 2);
		}
		layout->addLayout(actionLayout, 1);

		QFrame* secondaryPanel = new QFrame(surface);
		secondaryPanel->setObjectName("SecondaryPanel");
		QHBoxLayout* secondaryLayout = new QHBoxLayout(secondaryPanel);
		secondaryLayout->setContentsMargins(14, 12, 14, 12);
		secondaryLayout->setSpacing(10);
		secondaryLayout->addWidget(new QLabel("More", secondaryPanel));
		QComboBox* moreActions = new QComboBox(secondaryPanel);
		for (const WorkflowDefinition& workflow : CreateWorkflowDefinitions())
		{
			for (const QString& operationId : workflow.OperationIds)
			{
				moreActions->addItem(DisplayNameForOperation(operationId), operationId);
			}
		}
		connect(moreActions, &QComboBox::currentTextChanged, [moreActions, this]() {
			SetSelectedOperation(moreActions->currentData().toString());
		});
		secondaryLayout->addWidget(moreActions, 1);

		QPushButton* detailsButton = new QPushButton("Details", secondaryPanel);
		detailsButton->setObjectName("SecondaryButton");
		detailsButton->setToolTip("Preview the selected action before running it.");
		connect(detailsButton, &QPushButton::clicked, this, &LauncherMainWindow::PreviewSelectedOperation);
		secondaryLayout->addWidget(detailsButton);

		QPushButton* runMoreButton = new QPushButton("Run Selected", secondaryPanel);
		runMoreButton->setObjectName("PrimaryActionButton");
		connect(runMoreButton, &QPushButton::clicked, this, &LauncherMainWindow::RunSelectedOperation);
		secondaryLayout->addWidget(runMoreButton);
		layout->addWidget(secondaryPanel);
		return surface;
	}

	QPushButton* LauncherMainWindow::CreateWorkflowButton(const WorkflowDefinition& workflow, int index)
	{
		QPushButton* button = new QPushButton(workflow.Title + "\n" + workflow.Subtitle, this);
		button->setObjectName("WorkflowButton");
		button->setCheckable(true);
		button->setToolTip("Show " + workflow.Title + " operations.");
		button->setMinimumHeight(82);
		button->setChecked(index == 0);
		return button;
	}

	QWidget* LauncherMainWindow::CreateWorkflowDetailPage(const WorkflowDefinition& workflow)
	{
		QFrame* page = new QFrame(this);
		page->setObjectName("WorkflowDetail");
		QVBoxLayout* layout = new QVBoxLayout(page);
		layout->setContentsMargins(18, 18, 18, 18);
		layout->setSpacing(10);
		layout->addWidget(CreateSectionLabel(workflow.Title));

		for (int index = 0; index < workflow.OperationIds.size(); ++index)
		{
			AddOperationChoice(*layout, workflow.OperationIds[index], index == 0);
		}

		if (workflow.Title == "Build")
		{
			layout->addWidget(CreateBoundLineEdit("Optional build targets, comma separated", "Equivalent to repeating --target.", &LauncherSettings::SetSelectedTargets));
			layout->addWidget(CreateBoundCheckBox("Force configure before build", "Equivalent to --force-configure.", &LauncherSettings::SetForceConfigure));
		}
		else if (workflow.Title == "Cook")
		{
			layout->addWidget(CreateBoundLineEdit("Optional shader package ids, comma separated", "Equivalent to repeating --shader-package.", &LauncherSettings::SetShaderPackages));
			layout->addWidget(CreateBoundCheckBox("Force recook", "Equivalent to --force-recook.", &LauncherSettings::SetForceRecook));
			layout->addWidget(CreateBoundCheckBox("Confirm force recook cleanup", "Required before destructive force recook runs.", &LauncherSettings::SetConfirmForceRecook));
		}
		else if (workflow.Title == "Validate")
		{
			layout->addWidget(CreateBoundLineEdit("Validation groups: aggregate, boundaries, parity, readiness", "Equivalent to repeating --validation-group.", &LauncherSettings::SetValidationGroups));
			layout->addWidget(CreateBoundLineEdit("Validation targets, comma separated", "Equivalent to repeating --validation-target.", &LauncherSettings::SetValidationTargets));
			layout->addWidget(CreateBoundLineEdit("Smoke backend, for example d3d12 or vulkan", "Equivalent to --smoke-backend.", &LauncherSettings::SetSmokeBackend));
			layout->addWidget(CreateBoundLineEdit("Smoke frame limit", "Equivalent to --smoke-frame-limit.", &LauncherSettings::SetSmokeFrameLimit));
			layout->addWidget(CreateBoundCheckBox("Enable smoke trace", "Equivalent to --smoke-trace.", &LauncherSettings::SetSmokeTrace));
			layout->addWidget(CreateBoundCheckBox("Skip smoke level switching", "Equivalent to --smoke-skip-level-switching.", &LauncherSettings::SetSmokeSkipLevelSwitching));

			QComboBox* formatModeBox = new QComboBox(page);
			formatModeBox->addItem("Check formatting", "check");
			formatModeBox->addItem("Apply formatting", "apply");
			formatModeBox->setToolTip("Equivalent to --format-mode.");
			connect(formatModeBox, &QComboBox::currentTextChanged, [formatModeBox, this]() {
				m_settings.SetFormatMode(formatModeBox->currentData().toString());
			});
			layout->addWidget(formatModeBox);
		}
		else if (workflow.Title == "Workspace")
		{
			QComboBox* cleanScopeBox = new QComboBox(page);
			cleanScopeBox->addItem("Selected Project Cooked Outputs", "selected-cooked");
			cleanScopeBox->addItem("All Cooked Outputs", "all-cooked");
			cleanScopeBox->addItem("Build Tree", "build-tree");
			cleanScopeBox->addItem("Shader Cache", "shader-cache");
			cleanScopeBox->addItem("Third-Party Dependency Cache", "deps");
			cleanScopeBox->addItem("Logs", "logs");
			cleanScopeBox->addItem("Pristine Generated Workspace", "pristine");
			cleanScopeBox->setToolTip("Equivalent to --clean-scope.");
			connect(cleanScopeBox, &QComboBox::currentTextChanged, [cleanScopeBox, this]() {
				m_settings.SetCleanScope(cleanScopeBox->currentData().toString());
			});
			layout->addWidget(cleanScopeBox);
			layout->addWidget(CreateBoundCheckBox("Confirm clean operation", "Required before destructive clean scopes run.", &LauncherSettings::SetConfirmClean));
		}

		layout->addStretch(1);
		return page;
	}

	QWidget* LauncherMainWindow::CreateAdvancedDrawer()
	{
		m_advancedDrawer = new QFrame(this);
		m_advancedDrawer->setObjectName("AdvancedDrawer");
		m_advancedDrawer->setFixedWidth(300);
		m_advancedDrawer->setVisible(false);

		QVBoxLayout* layout = new QVBoxLayout(m_advancedDrawer);
		layout->setContentsMargins(18, 18, 18, 18);
		layout->setSpacing(10);
		layout->addWidget(CreatePageTitle("Options", "Only change these when needed.", m_advancedDrawer));

		layout->addWidget(CreateSectionLabel("Profiles"));
		QComboBox* editorProfileBox = new QComboBox(m_advancedDrawer);
		editorProfileBox->addItems({"DebugEditor", "DevelopmentEditor", "ShippingEditor"});
		editorProfileBox->setCurrentText(m_settings.EditorProfile());
		connect(editorProfileBox, &QComboBox::currentTextChanged, &m_settings, &LauncherSettings::SetEditorProfile);
		layout->addWidget(editorProfileBox);

		QComboBox* runtimeProfileBox = new QComboBox(m_advancedDrawer);
		runtimeProfileBox->addItems({"DebugGame", "DevelopmentGame", "ShippingGame"});
		runtimeProfileBox->setCurrentText(m_settings.RuntimeProfile());
		connect(runtimeProfileBox, &QComboBox::currentTextChanged, &m_settings, &LauncherSettings::SetRuntimeProfile);
		layout->addWidget(runtimeProfileBox);

		layout->addWidget(CreateSectionLabel("Build"));
		layout->addWidget(CreateBoundLineEdit("Targets", "Optional build target filter.", &LauncherSettings::SetSelectedTargets));
		layout->addWidget(CreateBoundCheckBox("Force configure", "Regenerate before building.", &LauncherSettings::SetForceConfigure));

		layout->addWidget(CreateSectionLabel("Cook"));
		layout->addWidget(CreateBoundLineEdit("Shader packages", "Optional shader package ids.", &LauncherSettings::SetShaderPackages));
		layout->addWidget(CreateBoundCheckBox("Force recook", "Clean and recook instead of incremental cook.", &LauncherSettings::SetForceRecook));
		layout->addWidget(CreateBoundCheckBox("Confirm recook cleanup", "Required before destructive force recook runs.", &LauncherSettings::SetConfirmForceRecook));

		layout->addWidget(CreateSectionLabel("Validation"));
		layout->addWidget(CreateBoundLineEdit("Groups", "Optional validation groups.", &LauncherSettings::SetValidationGroups));
		layout->addWidget(CreateBoundLineEdit("Targets", "Optional validation targets.", &LauncherSettings::SetValidationTargets));

		layout->addWidget(CreateSectionLabel("Smoke"));
		layout->addWidget(CreateBoundLineEdit("Backend", "Optional smoke backend.", &LauncherSettings::SetSmokeBackend));
		layout->addWidget(CreateBoundLineEdit("Frame limit", "Optional smoke frame limit.", &LauncherSettings::SetSmokeFrameLimit));

		layout->addWidget(CreateSectionLabel("Clean"));
		QComboBox* cleanScopeBox = new QComboBox(m_advancedDrawer);
		cleanScopeBox->addItem("Selected Project Cooked Outputs", "selected-cooked");
		cleanScopeBox->addItem("All Cooked Outputs", "all-cooked");
		cleanScopeBox->addItem("Build Tree", "build-tree");
		cleanScopeBox->addItem("Shader Cache", "shader-cache");
		cleanScopeBox->addItem("Third-Party Dependency Cache", "deps");
		cleanScopeBox->addItem("Logs", "logs");
		cleanScopeBox->addItem("Pristine Generated Workspace", "pristine");
		connect(cleanScopeBox, &QComboBox::currentTextChanged, [cleanScopeBox, this]() {
			m_settings.SetCleanScope(cleanScopeBox->currentData().toString());
		});
		layout->addWidget(cleanScopeBox);
		layout->addWidget(CreateBoundCheckBox("Confirm clean", "Required before destructive clean scopes run.", &LauncherSettings::SetConfirmClean));
		layout->addStretch(1);
		return m_advancedDrawer;
	}

	QWidget* LauncherMainWindow::CreateOutputPanel()
	{
		QFrame* panel = new QFrame(this);
		panel->setObjectName("OutputPanel");
		QVBoxLayout* layout = new QVBoxLayout(panel);
		layout->setContentsMargins(18, 12, 18, 14);
		layout->setSpacing(6);

		QHBoxLayout* toolbar = new QHBoxLayout();
		toolbar->setSpacing(10);
		m_activeOperationLabel = new QLabel("Ready", panel);
		m_activeOperationLabel->setObjectName("ActiveOperationLabel");
		toolbar->addWidget(m_activeOperationLabel);
		toolbar->addStretch(1);
		layout->addLayout(toolbar);

		m_operationOutput = new QTextEdit(panel);
		m_operationOutput->setObjectName("OperationOutput");
		m_operationOutput->setReadOnly(true);
		m_operationOutput->setMinimumHeight(92);
		m_operationOutput->setMaximumHeight(130);
		m_operationOutput->setToolTip("Action status and optional details appear here.");
		m_operationOutput->setPlainText("Choose an action above. Use Details only when you want to inspect the plan before running.");
		layout->addWidget(m_operationOutput);
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

	void LauncherMainWindow::AddOperationChoice(QVBoxLayout& layout, const QString& operationId, bool checked)
	{
		QRadioButton* button = new QRadioButton(DisplayNameForOperation(operationId), this);
		button->setProperty("OperationId", operationId);
		button->setToolTip(operationId);
		button->setChecked(checked);
		m_operationButtonGroup->addButton(button);
		layout.addWidget(button);
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
		request.ValidationGroups = m_settings.ValidationGroups();
		request.ValidationTargets = m_settings.ValidationTargets();
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
		    "Confirm Launcher Operation",
		    "This operation has a confirmed destructive option enabled. Continue?",
		    QMessageBox::Yes | QMessageBox::No,
		    QMessageBox::No);
		return result == QMessageBox::Yes;
	}

	void LauncherMainWindow::SetStatusMessage(const QString& message)
	{
		if (m_statusLabel != nullptr)
		{
			m_statusLabel->setText(message);
		}
		statusBar()->showMessage(message);
	}

	void LauncherMainWindow::SetOperationControlsEnabled(bool enabled)
	{
		if (m_previewButton != nullptr)
		{
			m_previewButton->setEnabled(enabled);
		}
		if (m_runButton != nullptr)
		{
			m_runButton->setEnabled(enabled);
		}
		if (m_projectList != nullptr)
		{
			m_projectList->setEnabled(enabled);
		}
		if (m_workflowButtonGroup != nullptr)
		{
			for (QAbstractButton* button : m_workflowButtonGroup->buttons())
			{
				button->setEnabled(enabled);
			}
		}
		if (m_operationButtonGroup != nullptr)
		{
			for (QAbstractButton* button : m_operationButtonGroup->buttons())
			{
				button->setEnabled(enabled);
			}
		}
	}

	void LauncherMainWindow::SetSelectedOperation(const QString& operationId)
	{
		m_selectedOperationId = operationId;
		if (m_operationButtonGroup != nullptr)
		{
			for (QAbstractButton* button : m_operationButtonGroup->buttons())
			{
				if (button->property("OperationId").toString() == operationId)
				{
					button->setChecked(true);
					break;
				}
			}
		}

		const QString title = DisplayNameForOperation(operationId);
		if (m_activeOperationLabel != nullptr)
		{
			m_activeOperationLabel->setText("Selected: " + title);
		}
		if (m_runButton != nullptr)
		{
			m_runButton->setText("Run " + title);
		}
	}

	void LauncherMainWindow::PopulateProjects()
	{
		m_projectList->clear();
		for (const LauncherProjectSummary& project : m_projectModel.Projects())
		{
			QListWidgetItem* item = new QListWidgetItem(project.DisplayName, m_projectList);
			item->setData(Qt::UserRole, project.Id);
			item->setToolTip(project.Id + "\n" + QString::fromStdString(project.RootPath.string()));
			if (project.Id == m_projectModel.SelectedProjectId())
			{
				m_projectList->setCurrentItem(item);
			}
		}

		if (m_projectModel.Projects().empty())
		{
			m_projectList->addItem("No Sparkle projects discovered.");
		}
	}

	QVector<LauncherMainWindow::WorkflowDefinition> LauncherMainWindow::CreateWorkflowDefinitions() const
	{
		return {
		    {"Build", "Editor / Runtime", {"project.build.editor", "project.build.runtime", "workspace.generate-solution", "toolchain.check"}},
		    {"Cook", "Assets / Shaders", {"cook.tools.prepare", "cook.project", "cook.shaders", "cook.textures", "cook.assets"}},
		    {"Launch", "Editor / Runtime", {"project.launch.editor", "project.launch.runtime"}},
		    {"Validate", "Format / Checks", {"quality.validate", "quality.format", "smoke.rhi.editor", "smoke.rhi.runtime"}},
		    {"Workspace", "Setup / Clean", {"workspace.setup", "workspace.clean"}},
		};
	}

	void LauncherMainWindow::ApplyVisualStyle()
	{
		setStyleSheet(
		    "QMainWindow, QWidget { background: #15171c; color: #e8edf2; font-family: 'Segoe UI'; font-size: 10pt; }"
		    "#Header { background: #101217; border-bottom: 1px solid #2b3038; }"
		    "#ProjectRail { background: #11141a; border-right: 1px solid #2b3038; }"
		    "#WorkflowSurface { background: #15171c; }"
		    "#AdvancedDrawer { background: #11141a; border-left: 1px solid #2b3038; }"
		    "#OutputPanel { background: #101217; border-top: 1px solid #2b3038; }"
		    "#ProductLabel { color: #ffffff; font-size: 20pt; font-weight: 700; }"
		    "#StatusLabel { color: #98a6ba; background: #181c23; border: 1px solid #2d3440; border-radius: 6px; padding: 10px; }"
		    "#ActiveOperationLabel { color: #ffffff; font-size: 11pt; font-weight: 700; }"
		    "QStatusBar { background: #101217; color: #8f9bae; border-top: 1px solid #2b3038; }"
		    "QListWidget { background: #1d222b; border: 1px solid #303743; border-radius: 6px; padding: 6px; outline: 0; }"
		    "QListWidget::item { padding: 10px; border-radius: 4px; }"
		    "QListWidget::item:selected { background: #2f6fed; color: #ffffff; }"
		    "#SecondaryPanel { background: #181c23; border: 1px solid #2c3440; border-radius: 6px; }"
		    "#WorkflowButton, #PrimaryWorkflowButton { color: #ffffff; border: 1px solid #303743; border-radius: 6px; padding: 16px; text-align: left; font-size: 13pt; font-weight: 700; }"
		    "#WorkflowButton { background: #1d222b; }"
		    "#WorkflowButton:hover { background: #242a35; border: 1px solid #5a677a; }"
		    "#PrimaryWorkflowButton { background: #1f9d78; border: 1px solid #26b98e; }"
		    "#PrimaryWorkflowButton:hover { background: #26b98e; }"
		    "#PageTitle h1 { color: #ffffff; font-size: 18pt; margin: 0; }"
		    "#PageTitle p { color: #9da9bb; margin-top: 4px; }"
		    "#SectionLabel { color: #ffffff; font-size: 11pt; font-weight: 700; padding-top: 6px; }"
		    "QPushButton { background: #2f6fed; color: #ffffff; border: none; border-radius: 5px; padding: 9px 14px; font-weight: 600; }"
		    "QPushButton:hover { background: #3d7bff; }"
		    "#SecondaryButton { background: #262d38; color: #d8e0ea; }"
		    "#SecondaryButton:hover { background: #303846; }"
		    "#PrimaryActionButton { background: #1f9d78; }"
		    "#PrimaryActionButton:hover { background: #26b98e; }"
		    "QComboBox, QLineEdit, QTextEdit { background: #1d222b; border: 1px solid #303743; border-radius: 6px; padding: 8px; color: #e8edf2; }"
		    "QComboBox:focus, QLineEdit:focus, QTextEdit:focus { border: 1px solid #5f8cff; }"
		    "QRadioButton, QCheckBox { spacing: 8px; padding: 4px; }"
		    "QLabel { color: #c7d0de; }");
	}
}