#pragma once

#include <QtCore/QString>
#include <QtCore/QVector>
#include <QtWidgets/QAbstractButton>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QFrame>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QStackedWidget>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QVBoxLayout>

#include <filesystem>

namespace SparkleLauncher
{
	class LauncherBackend;
	struct LauncherOperationDescriptor;
	struct LauncherOperationRequest;
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
		void RefreshProjects();
		void SelectProjectFromList();
		void SelectWorkflow(int index);
		void SelectOperationButton(QAbstractButton* button);
		void ToggleAdvancedDrawer();
		void PreviewSelectedOperation();
		void RunSelectedOperation();
		void DisplayOperationPreview(const QString& operationId, const QString& title, const QString& previewText, bool canRun);
		void DisplayOperationPreviewError(const QString& operationId, const QString& message);
		void DisplayOperationStarted(const QString& operationId, const QString& title);
		void AppendOperationOutput(const QString& operationId, const QString& outputText);
		void DisplayOperationFinished(const QString& operationId, const QString& title, const QString& statusText, int exitCode);

	private:
		struct WorkflowDefinition
		{
			QString Title;
			QString Subtitle;
			QVector<QString> OperationIds;
		};

		QWidget* CreateHeader();
		QWidget* CreateProjectRail();
		QWidget* CreateWorkflowSurface();
		QPushButton* CreateWorkflowButton(const WorkflowDefinition& workflow, int index);
		QWidget* CreateWorkflowDetailPage(const WorkflowDefinition& workflow);
		QWidget* CreateAdvancedDrawer();
		QWidget* CreateOutputPanel();
		QLabel* CreatePageTitle(const QString& title, const QString& subtitle, QWidget* parent = nullptr) const;
		QLabel* CreateSectionLabel(const QString& title) const;
		QLineEdit* CreateBoundLineEdit(const QString& placeholder, const QString& tooltip, void (LauncherSettings::*setter)(const QString&));
		QCheckBox* CreateBoundCheckBox(const QString& label, const QString& tooltip, void (LauncherSettings::*setter)(bool));
		void AddOperationChoice(QVBoxLayout& layout, const QString& operationId, bool checked);
		const LauncherOperationDescriptor* FindOperationDescriptor(const QString& operationId) const;
		QString DisplayNameForOperation(const QString& operationId) const;
		LauncherOperationRequest BuildOperationRequest(const QString& operationId) const;
		bool ConfirmRunRequest(const LauncherOperationRequest& request) const;
		void SetStatusMessage(const QString& message);
		void SetOperationControlsEnabled(bool enabled);
		void SetSelectedOperation(const QString& operationId);
		void PopulateProjects();
		QVector<WorkflowDefinition> CreateWorkflowDefinitions() const;
		void ApplyVisualStyle();

		std::filesystem::path m_repositoryRoot;
		LauncherProjectModel& m_projectModel;
		LauncherSettings& m_settings;
		LauncherBackend& m_backend;
		QButtonGroup* m_workflowButtonGroup = nullptr;
		QButtonGroup* m_operationButtonGroup = nullptr;
		QStackedWidget* m_workflowStack = nullptr;
		QListWidget* m_projectList = nullptr;
		QFrame* m_advancedDrawer = nullptr;
		QPushButton* m_advancedButton = nullptr;
		QTextEdit* m_operationOutput = nullptr;
		QPushButton* m_previewButton = nullptr;
		QPushButton* m_runButton = nullptr;
		QLabel* m_activeOperationLabel = nullptr;
		QLabel* m_statusLabel = nullptr;
		QString m_selectedOperationId;
		bool m_operationInProgress = false;
	};
}