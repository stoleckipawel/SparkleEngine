#include "LauncherMainWindow.h"

#include "LauncherBackend.h"
#include "LauncherProjectModel.h"
#include "LauncherSettings.h"

#include <QtCore/Qt>
#include <QtGui/QTextCursor>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QFrame>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QScrollArea>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

#include <utility>

namespace SparkleLauncher
{
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
		resize(1280, 760);

		QWidget* centralWidget = new QWidget(this);
		QHBoxLayout* rootLayout = new QHBoxLayout(centralWidget);
		rootLayout->setContentsMargins(0, 0, 0, 0);
		rootLayout->setSpacing(0);

		m_pageStack = new QStackedWidget(centralWidget);
		m_pageStack->addWidget(CreateProjectsPage());
		m_pageStack->addWidget(CreateOperationsPage());
		m_pageStack->addWidget(CreateSettingsPage());
		m_pageStack->addWidget(CreateAboutPage());

		rootLayout->addWidget(CreateSidebar());
		rootLayout->addWidget(m_pageStack, 1);
		setCentralWidget(centralWidget);

		ApplyVisualStyle();
		PopulateOperations();

		connect(m_navigationList, &QListWidget::currentRowChanged, this, &LauncherMainWindow::ShowNavigationPage);
		connect(&m_projectModel, &LauncherProjectModel::ProjectsChanged, this, &LauncherMainWindow::PopulateProjects);
		connect(&m_projectModel, &LauncherProjectModel::ProjectDiscoveryFailed, this, &LauncherMainWindow::SetStartupNotice);
		connect(&m_backend, &LauncherBackend::OperationPreviewReady, this, &LauncherMainWindow::DisplayOperationPreview);
		connect(&m_backend, &LauncherBackend::OperationPreviewFailed, this, &LauncherMainWindow::DisplayOperationPreviewError);
		connect(&m_backend, &LauncherBackend::OperationStarted, this, &LauncherMainWindow::DisplayOperationStarted);
		connect(&m_backend, &LauncherBackend::OperationOutputReceived, this, &LauncherMainWindow::AppendOperationOutput);
		connect(&m_backend, &LauncherBackend::OperationFinished, this, &LauncherMainWindow::DisplayOperationFinished);

		m_navigationList->setCurrentRow(0);
		RefreshProjects();
	}

	void LauncherMainWindow::SetStartupNotice(const QString& message)
	{
		if (message.isEmpty() || m_statusLabel == nullptr)
		{
			return;
		}

		m_statusLabel->setText(message);
	}

	void LauncherMainWindow::ShowNavigationPage(int index)
	{
		if (m_pageStack != nullptr && index >= 0 && index < m_pageStack->count())
		{
			m_pageStack->setCurrentIndex(index);
		}
	}

	void LauncherMainWindow::RefreshProjects()
	{
		m_projectModel.Refresh(m_repositoryRoot);
	}

	void LauncherMainWindow::SelectProjectFromList()
	{
		QListWidgetItem* selectedItem = m_projectList == nullptr ? nullptr : m_projectList->currentItem();
		if (selectedItem == nullptr)
		{
			return;
		}

		m_projectModel.SelectProject(selectedItem->data(Qt::UserRole).toString());
	}

	void LauncherMainWindow::PreviewSelectedOperation()
	{
		QListWidgetItem* selectedItem = m_operationList == nullptr ? nullptr : m_operationList->currentItem();
		if (selectedItem == nullptr)
		{
			m_operationOutput->setPlainText("Select an operation to preview the native launcher backend plan.");
			return;
		}

		m_backend.RequestOperationPreview(BuildOperationRequest(selectedItem->data(Qt::UserRole).toString()));
	}

	void LauncherMainWindow::RunSelectedOperation()
	{
		QListWidgetItem* selectedItem = m_operationList == nullptr ? nullptr : m_operationList->currentItem();
		if (selectedItem == nullptr)
		{
			m_operationOutput->setPlainText("Select an operation before running a native launcher workflow.");
			return;
		}

		m_backend.RunOperation(BuildOperationRequest(selectedItem->data(Qt::UserRole).toString()));
	}

	void LauncherMainWindow::DisplayOperationPreview(const QString&, const QString& title, const QString& previewText, bool canRun)
	{
		m_operationOutput->setPlainText(title + QString(canRun ? " [Ready]" : " [Blocked]") + "\n\n" + previewText);
	}

	void LauncherMainWindow::DisplayOperationPreviewError(const QString&, const QString& message)
	{
		m_operationOutput->setPlainText(message);
	}

	void LauncherMainWindow::DisplayOperationStarted(const QString&, const QString& title)
	{
		m_operationOutput->setPlainText(title + "\n\nOperation started. Output will stream here.\n");
	}

	void LauncherMainWindow::AppendOperationOutput(const QString&, const QString& outputText)
	{
		m_operationOutput->moveCursor(QTextCursor::End);
		m_operationOutput->insertPlainText(outputText);
		m_operationOutput->moveCursor(QTextCursor::End);
	}

	void LauncherMainWindow::DisplayOperationFinished(const QString&, const QString& title, const QString& statusText, int)
	{
		m_operationOutput->append("\n" + title + " finished: " + statusText);
	}

	QWidget* LauncherMainWindow::CreateSidebar()
	{
		QFrame* sidebar = new QFrame(this);
		sidebar->setObjectName("Sidebar");
		sidebar->setFixedWidth(232);

		QVBoxLayout* sidebarLayout = new QVBoxLayout(sidebar);
		sidebarLayout->setContentsMargins(18, 22, 18, 18);
		sidebarLayout->setSpacing(18);

		QLabel* productLabel = new QLabel("Sparkle", sidebar);
		productLabel->setObjectName("ProductLabel");
		QLabel* modeLabel = new QLabel("Launcher", sidebar);
		modeLabel->setObjectName("ModeLabel");

		m_navigationList = new QListWidget(sidebar);
		m_navigationList->setObjectName("NavigationList");
		m_navigationList->addItem("Projects");
		m_navigationList->addItem("Operations");
		m_navigationList->addItem("Settings");
		m_navigationList->addItem("About");

		m_statusLabel = new QLabel("Phase 1 Qt shell ready", sidebar);
		m_statusLabel->setObjectName("StatusLabel");
		m_statusLabel->setWordWrap(true);

		sidebarLayout->addWidget(productLabel);
		sidebarLayout->addWidget(modeLabel);
		sidebarLayout->addWidget(m_navigationList, 1);
		sidebarLayout->addWidget(m_statusLabel);
		return sidebar;
	}

	QWidget* LauncherMainWindow::CreateProjectsPage()
	{
		QWidget* page = new QWidget(this);
		QVBoxLayout* layout = new QVBoxLayout(page);
		layout->setContentsMargins(34, 30, 34, 30);
		layout->setSpacing(18);

		layout->addWidget(CreatePageTitle("Projects", "Open and manage Sparkle projects from one launcher surface."));

		m_projectList = new QListWidget(page);
		m_projectList->setObjectName("ProjectList");
		connect(m_projectList, &QListWidget::currentItemChanged, this, &LauncherMainWindow::SelectProjectFromList);

		QPushButton* refreshButton = new QPushButton("Refresh Projects", page);
		connect(refreshButton, &QPushButton::clicked, this, &LauncherMainWindow::RefreshProjects);

		layout->addWidget(m_projectList, 1);
		layout->addWidget(refreshButton, 0, Qt::AlignLeft);
		return page;
	}

	QWidget* LauncherMainWindow::CreateOperationsPage()
	{
		QWidget* page = new QWidget(this);
		QVBoxLayout* layout = new QVBoxLayout(page);
		layout->setContentsMargins(34, 30, 34, 30);
		layout->setSpacing(18);

		layout->addWidget(CreatePageTitle("Operations", "Build, cook, maintain, and launch workflows will be driven here."));

		QHBoxLayout* operationLayout = new QHBoxLayout();
		operationLayout->setSpacing(16);

		m_operationList = new QListWidget(page);
		m_operationList->setObjectName("OperationList");

		m_operationOutput = new QTextEdit(page);
		m_operationOutput->setObjectName("OperationOutput");
		m_operationOutput->setReadOnly(true);
		m_operationOutput->setPlainText("Select an operation to preview the native launcher backend plan.");

		operationLayout->addWidget(m_operationList, 1);
		operationLayout->addWidget(m_operationOutput, 2);

		QPushButton* previewButton = new QPushButton("Preview Operation", page);
		connect(previewButton, &QPushButton::clicked, this, &LauncherMainWindow::PreviewSelectedOperation);
		QPushButton* runButton = new QPushButton("Run Operation", page);
		connect(runButton, &QPushButton::clicked, this, &LauncherMainWindow::RunSelectedOperation);

		QHBoxLayout* buttonLayout = new QHBoxLayout();
		buttonLayout->setSpacing(10);
		buttonLayout->addWidget(previewButton);
		buttonLayout->addWidget(runButton);
		buttonLayout->addStretch(1);

		layout->addLayout(operationLayout, 1);
		layout->addLayout(buttonLayout);
		return page;
	}

	QWidget* LauncherMainWindow::CreateSettingsPage()
	{
		QWidget* page = new QWidget(this);
		QVBoxLayout* pageLayout = new QVBoxLayout(page);
		pageLayout->setContentsMargins(34, 30, 34, 30);
		pageLayout->setSpacing(18);

		pageLayout->addWidget(CreatePageTitle("Settings", "Profiles and launcher preferences stay inside the Qt shell."));

		QScrollArea* scrollArea = new QScrollArea(page);
		scrollArea->setWidgetResizable(true);
		QWidget* settingsContent = new QWidget(scrollArea);
		QVBoxLayout* layout = new QVBoxLayout(settingsContent);
		layout->setContentsMargins(0, 0, 12, 0);
		layout->setSpacing(12);

		QComboBox* editorProfileBox = new QComboBox(page);
		editorProfileBox->addItems({"DebugEditor", "DevelopmentEditor", "ShippingEditor"});
		editorProfileBox->setCurrentText(m_settings.EditorProfile());
		connect(editorProfileBox, &QComboBox::currentTextChanged, &m_settings, &LauncherSettings::SetEditorProfile);

		QComboBox* runtimeProfileBox = new QComboBox(page);
		runtimeProfileBox->addItems({"DebugGame", "DevelopmentGame", "ShippingGame"});
		runtimeProfileBox->setCurrentText(m_settings.RuntimeProfile());
		connect(runtimeProfileBox, &QComboBox::currentTextChanged, &m_settings, &LauncherSettings::SetRuntimeProfile);

		layout->addWidget(new QLabel("Editor profile", page));
		layout->addWidget(editorProfileBox);
		layout->addWidget(new QLabel("Runtime profile", page));
		layout->addWidget(runtimeProfileBox);

		QCheckBox* forceConfigureBox = new QCheckBox("Force configure before build operations", page);
		connect(forceConfigureBox, &QCheckBox::toggled, &m_settings, &LauncherSettings::SetForceConfigure);
		QCheckBox* forceRecookBox = new QCheckBox("Force recook mode", page);
		connect(forceRecookBox, &QCheckBox::toggled, &m_settings, &LauncherSettings::SetForceRecook);
		QCheckBox* confirmForceRecookBox = new QCheckBox("Confirm force recook cleanup", page);
		connect(confirmForceRecookBox, &QCheckBox::toggled, &m_settings, &LauncherSettings::SetConfirmForceRecook);
		QCheckBox* confirmCleanBox = new QCheckBox("Confirm clean operation", page);
		connect(confirmCleanBox, &QCheckBox::toggled, &m_settings, &LauncherSettings::SetConfirmClean);
		QCheckBox* smokeTraceBox = new QCheckBox("Enable smoke trace", page);
		connect(smokeTraceBox, &QCheckBox::toggled, &m_settings, &LauncherSettings::SetSmokeTrace);
		QCheckBox* smokeSkipLevelSwitchingBox = new QCheckBox("Skip smoke level switching", page);
		connect(smokeSkipLevelSwitchingBox, &QCheckBox::toggled, &m_settings, &LauncherSettings::SetSmokeSkipLevelSwitching);

		QLineEdit* selectedTargetsEdit = new QLineEdit(page);
		selectedTargetsEdit->setPlaceholderText("Optional build targets, comma separated");
		connect(selectedTargetsEdit, &QLineEdit::textChanged, &m_settings, &LauncherSettings::SetSelectedTargets);
		QLineEdit* shaderPackagesEdit = new QLineEdit(page);
		shaderPackagesEdit->setPlaceholderText("Optional shader package ids, comma separated");
		connect(shaderPackagesEdit, &QLineEdit::textChanged, &m_settings, &LauncherSettings::SetShaderPackages);
		QLineEdit* validationGroupsEdit = new QLineEdit(page);
		validationGroupsEdit->setPlaceholderText("Validation groups: aggregate, boundaries, parity, readiness");
		connect(validationGroupsEdit, &QLineEdit::textChanged, &m_settings, &LauncherSettings::SetValidationGroups);
		QLineEdit* validationTargetsEdit = new QLineEdit(page);
		validationTargetsEdit->setPlaceholderText("Validation targets, comma separated");
		connect(validationTargetsEdit, &QLineEdit::textChanged, &m_settings, &LauncherSettings::SetValidationTargets);
		QLineEdit* smokeBackendEdit = new QLineEdit(page);
		smokeBackendEdit->setPlaceholderText("Smoke backend, for example d3d12 or vulkan");
		connect(smokeBackendEdit, &QLineEdit::textChanged, &m_settings, &LauncherSettings::SetSmokeBackend);
		QLineEdit* smokeFrameLimitEdit = new QLineEdit(page);
		smokeFrameLimitEdit->setPlaceholderText("Smoke frame limit");
		connect(smokeFrameLimitEdit, &QLineEdit::textChanged, &m_settings, &LauncherSettings::SetSmokeFrameLimit);

		QComboBox* formatModeBox = new QComboBox(page);
		formatModeBox->addItem("Check", "check");
		formatModeBox->addItem("Apply", "apply");
		connect(formatModeBox, &QComboBox::currentTextChanged, [formatModeBox, this]() {
			m_settings.SetFormatMode(formatModeBox->currentData().toString());
		});

		QComboBox* cleanScopeBox = new QComboBox(page);
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

		layout->addWidget(forceConfigureBox);
		layout->addWidget(forceRecookBox);
		layout->addWidget(confirmForceRecookBox);
		layout->addWidget(confirmCleanBox);
		layout->addWidget(smokeTraceBox);
		layout->addWidget(smokeSkipLevelSwitchingBox);
		layout->addWidget(new QLabel("Build targets", page));
		layout->addWidget(selectedTargetsEdit);
		layout->addWidget(new QLabel("Shader packages", page));
		layout->addWidget(shaderPackagesEdit);
		layout->addWidget(new QLabel("Validation groups", page));
		layout->addWidget(validationGroupsEdit);
		layout->addWidget(new QLabel("Validation targets", page));
		layout->addWidget(validationTargetsEdit);
		layout->addWidget(new QLabel("Smoke backend", page));
		layout->addWidget(smokeBackendEdit);
		layout->addWidget(new QLabel("Smoke frame limit", page));
		layout->addWidget(smokeFrameLimitEdit);
		layout->addWidget(new QLabel("Format mode", page));
		layout->addWidget(formatModeBox);
		layout->addWidget(new QLabel("Clean scope", page));
		layout->addWidget(cleanScopeBox);
		layout->addStretch(1);
		scrollArea->setWidget(settingsContent);
		pageLayout->addWidget(scrollArea, 1);
		return page;
	}

	QWidget* LauncherMainWindow::CreateAboutPage()
	{
		QWidget* page = new QWidget(this);
		QVBoxLayout* layout = new QVBoxLayout(page);
		layout->setContentsMargins(34, 30, 34, 30);
		layout->setSpacing(18);
		layout->addWidget(CreatePageTitle("About", "Qt Widgets foundation inspired by Unity Hub and Epic Launcher."));

		QLabel* body = new QLabel(
		    "Phase 1 establishes the launcher shell, navigation, project model, settings model, and backend adapter. "
		    "It intentionally keeps execution disabled until Phase 2 binds native SparkleLauncherCore workflows.",
		    page);
		body->setWordWrap(true);
		layout->addWidget(body);
		layout->addStretch(1);
		return page;
	}

	QLabel* LauncherMainWindow::CreatePageTitle(const QString& title, const QString& subtitle) const
	{
		QLabel* label = new QLabel("<h1>" + title + "</h1><p>" + subtitle + "</p>");
		label->setObjectName("PageTitle");
		label->setTextFormat(Qt::RichText);
		return label;
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

	void LauncherMainWindow::PopulateProjects()
	{
		m_projectList->clear();
		for (const LauncherProjectSummary& project : m_projectModel.Projects())
		{
			QListWidgetItem* item = new QListWidgetItem(project.DisplayName, m_projectList);
			item->setData(Qt::UserRole, project.Id);
			item->setToolTip(QString::fromStdString(project.RootPath.string()));
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

	void LauncherMainWindow::PopulateOperations()
	{
		m_operationList->clear();
		for (const LauncherOperationDescriptor& operation : m_backend.Operations())
		{
			QListWidgetItem* item = new QListWidgetItem(operation.DisplayName, m_operationList);
			item->setData(Qt::UserRole, operation.Id);
			item->setToolTip(operation.Description);
		}
	}

	void LauncherMainWindow::ApplyVisualStyle()
	{
		setStyleSheet(
		    "QMainWindow, QWidget { background: #16181d; color: #e8edf2; font-family: 'Segoe UI'; font-size: 10pt; }"
		    "#Sidebar { background: #101217; border-right: 1px solid #2b3038; }"
		    "#ProductLabel { color: #ffffff; font-size: 23pt; font-weight: 700; }"
		    "#ModeLabel { color: #7f8da3; font-size: 10pt; text-transform: uppercase; }"
		    "#StatusLabel { color: #98a6ba; background: #181c23; border: 1px solid #2d3440; border-radius: 6px; padding: 10px; }"
		    "QListWidget { background: #1d222b; border: 1px solid #303743; border-radius: 6px; padding: 6px; outline: 0; }"
		    "QListWidget::item { padding: 10px; border-radius: 4px; }"
		    "QListWidget::item:selected { background: #2f6fed; color: #ffffff; }"
		    "#NavigationList { background: transparent; border: none; }"
		    "#NavigationList::item { padding: 12px 10px; }"
		    "#PageTitle h1 { color: #ffffff; font-size: 24pt; margin: 0; }"
		    "#PageTitle p { color: #9da9bb; margin-top: 6px; }"
		    "QPushButton { background: #2f6fed; color: #ffffff; border: none; border-radius: 5px; padding: 9px 14px; font-weight: 600; }"
		    "QPushButton:hover { background: #3d7bff; }"
		    "QComboBox, QTextEdit { background: #1d222b; border: 1px solid #303743; border-radius: 6px; padding: 8px; color: #e8edf2; }"
		    "QLabel { color: #c7d0de; }");
	}
}