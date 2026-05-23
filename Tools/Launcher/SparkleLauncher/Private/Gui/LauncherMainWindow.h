#pragma once

#include <QtCore/QString>
#include <QtCore/QVector>
#include <QtCore/QHash>
#include <QtWidgets/QAbstractButton>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QFrame>
#include <QtWidgets/QLabel>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QProgressBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QStackedWidget>
#include <QtWidgets/QTabWidget>
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
		void SelectProcessButton(QAbstractButton* button);
		void DisplaySelectedRunOutput(QListWidgetItem* currentItem, QListWidgetItem* previousItem);
		void RunSelectedOperation();
		void DisplayOperationStarted(const QString& runId, const QString& operationId, const QString& title);
		void AppendOperationOutput(const QString& runId, const QString& operationId, const QString& outputText);
		void DisplayOperationFinished(const QString& runId, const QString& operationId, const QString& title, const QString& statusText, int exitCode);

	private:
		struct WorkflowDefinition
		{
			QString Title;
			QString Subtitle;
			QVector<QString> OperationIds;
		};

		QWidget* CreateAppHeader();
		QWidget* CreateWorkflowSurface();
		QWidget* CreateProcessPicker(QWidget* parent);
		QPushButton* CreateProcessButton(const QString& label, const QString& operationId, QWidget* parent);
		QWidget* CreateOptionsPanel(QWidget* parent);
		QWidget* CreateOptionsPage(const QString& operationId, QWidget* parent);
		QWidget* CreateOutputPanel();
		QLabel* CreatePageTitle(const QString& title, const QString& subtitle, QWidget* parent = nullptr) const;
		QLabel* CreateSectionLabel(const QString& title) const;
		QLabel* CreateFieldLabel(const QString& title) const;
		QCheckBox* CreateBoundCheckBox(const QString& label, const QString& tooltip, void (LauncherSettings::*setter)(bool));
		QComboBox* CreateProfileCombo(const QStringList& profiles, const QString& currentProfile, void (LauncherSettings::*setter)(const QString&));
		QComboBox* CreateProjectCombo();
		QComboBox* CreateValueCombo(const QVector<QPair<QString, QString>>& options, const QString& currentValue, void (LauncherSettings::*setter)(const QString&));
		void AddOptionsForOperation(QVBoxLayout& layout, const QString& operationId);
		void AddOptionField(QVBoxLayout& layout, const QString& label, QWidget* control);
		void AddOptionCheckBox(QVBoxLayout& layout, QCheckBox* checkBox);
		void AddNoOptionsMessage(QVBoxLayout& layout, const QString& text);
		void SetControlsEnabled(bool enabled);
		const LauncherOperationDescriptor* FindOperationDescriptor(const QString& operationId) const;
		QString DisplayNameForOperation(const QString& operationId) const;
		QString DescriptionForOperation(const QString& operationId) const;
		LauncherOperationRequest BuildOperationRequest(const QString& operationId) const;
		bool ConfirmRunRequest(const LauncherOperationRequest& request) const;
		void SetStatusMessage(const QString& message);
		void SetSelectedOperation(const QString& operationId);
		void RegisterRun(const QString& runId, const QString& title);
		void AppendRunOutput(const QString& runId, const QString& text);
		void ShowRunOutput(const QString& runId);
		void UpdateProgress();
		void PopulateProjectSelectors();
		void PopulateProjectCombo(QComboBox& combo) const;
		QVector<WorkflowDefinition> CreateWorkflowDefinitions() const;
		void ApplyVisualStyle();

		std::filesystem::path m_repositoryRoot;
		LauncherProjectModel& m_projectModel;
		LauncherSettings& m_settings;
		LauncherBackend& m_backend;
		QButtonGroup* m_processButtonGroup = nullptr;
		QTabWidget* m_categoryTabs = nullptr;
		QStackedWidget* m_optionsStack = nullptr;
		QHash<QString, int> m_optionsPageByOperation;
		QVector<QComboBox*> m_projectSelectors;
		QTextEdit* m_operationOutput = nullptr;
		QPushButton* m_runButton = nullptr;
		QLabel* m_activeOperationLabel = nullptr;
		QLabel* m_activeOperationDescription = nullptr;
		QProgressBar* m_progressBar = nullptr;
		QLabel* m_progressLabel = nullptr;
		QListWidget* m_activityList = nullptr;
		QHash<QString, QListWidgetItem*> m_runItems;
		QHash<QString, QString> m_runOutputs;
		QString m_activeRunId;
		QString m_selectedOperationId;
		int m_nextRunIndex = 0;
		int m_startedRunCount = 0;
		int m_finishedRunCount = 0;
	};
}