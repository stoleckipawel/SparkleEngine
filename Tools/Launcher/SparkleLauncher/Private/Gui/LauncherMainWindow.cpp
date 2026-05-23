#include "LauncherMainWindow.h"

#include "LauncherBackend.h"
#include "LauncherProjectModel.h"
#include "LauncherSettings.h"

#include <QtCore/Qt>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QFrame>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QPushButton>
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
			m_operationOutput->setPlainText("Select an operation to preview the Phase 1 backend adapter boundary.");
			return;
		}

		m_backend.RequestOperationPreview(selectedItem->data(Qt::UserRole).toString());
	}

	void LauncherMainWindow::DisplayOperationPreview(const QString&, const QString& title, const QString& previewText)
	{
		m_operationOutput->setPlainText(title + "\n\n" + previewText);
	}

	void LauncherMainWindow::DisplayOperationPreviewError(const QString&, const QString& message)
	{
		m_operationOutput->setPlainText(message);
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
		m_operationOutput->setPlainText("Select an operation to preview the Phase 1 backend adapter boundary.");

		operationLayout->addWidget(m_operationList, 1);
		operationLayout->addWidget(m_operationOutput, 2);

		QPushButton* previewButton = new QPushButton("Preview Operation", page);
		connect(previewButton, &QPushButton::clicked, this, &LauncherMainWindow::PreviewSelectedOperation);

		layout->addLayout(operationLayout, 1);
		layout->addWidget(previewButton, 0, Qt::AlignLeft);
		return page;
	}

	QWidget* LauncherMainWindow::CreateSettingsPage()
	{
		QWidget* page = new QWidget(this);
		QVBoxLayout* layout = new QVBoxLayout(page);
		layout->setContentsMargins(34, 30, 34, 30);
		layout->setSpacing(18);

		layout->addWidget(CreatePageTitle("Settings", "Profiles and launcher preferences stay inside the Qt shell."));

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
		layout->addStretch(1);
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