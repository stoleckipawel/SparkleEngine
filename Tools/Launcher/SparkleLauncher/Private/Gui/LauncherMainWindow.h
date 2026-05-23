#pragma once

#include <QtWidgets/QLabel>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QStackedWidget>
#include <QtWidgets/QTextEdit>

#include <filesystem>

namespace SparkleLauncher
{
	class LauncherBackend;
	class LauncherProjectModel;
	class LauncherSettings;

	class LauncherMainWindow final : public QMainWindow
	{
		Q_OBJECT

	public:
		LauncherMainWindow(
		    std::filesystem::path repositoryRoot,
		    LauncherProjectModel& projectModel,
		    LauncherSettings& settings,
		    LauncherBackend& backend,
		    QWidget* parent = nullptr);

		void SetStartupNotice(const QString& message);

	private slots:
		void ShowNavigationPage(int index);
		void RefreshProjects();
		void SelectProjectFromList();
		void PreviewSelectedOperation();
		void DisplayOperationPreview(const QString& operationId, const QString& title, const QString& previewText);
		void DisplayOperationPreviewError(const QString& operationId, const QString& message);

	private:
		QWidget* CreateSidebar();
		QWidget* CreateProjectsPage();
		QWidget* CreateOperationsPage();
		QWidget* CreateSettingsPage();
		QWidget* CreateAboutPage();
		QLabel* CreatePageTitle(const QString& title, const QString& subtitle) const;
		void PopulateProjects();
		void PopulateOperations();
		void ApplyVisualStyle();

		std::filesystem::path m_repositoryRoot;
		LauncherProjectModel& m_projectModel;
		LauncherSettings& m_settings;
		LauncherBackend& m_backend;
		QListWidget* m_navigationList = nullptr;
		QStackedWidget* m_pageStack = nullptr;
		QListWidget* m_projectList = nullptr;
		QListWidget* m_operationList = nullptr;
		QTextEdit* m_operationOutput = nullptr;
		QLabel* m_statusLabel = nullptr;
	};
}