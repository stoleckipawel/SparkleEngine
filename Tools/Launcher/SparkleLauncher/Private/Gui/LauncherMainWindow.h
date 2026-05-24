#pragma once

#include <QtCore/QString>
#include <QtCore/QVector>
#include <QtCore/QHash>
#include <QtGui/QColor>
#include <QtGui/QIcon>
#include <QtWidgets/QAbstractButton>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QFrame>
#include <QtWidgets/QLabel>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QProgressBar>
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
		void SelectWorkflowGroupButton(QAbstractButton* button);
		void SelectProcessButton(QAbstractButton* button);
		void DisplaySelectedRunOutput(QListWidgetItem* currentItem, QListWidgetItem* previousItem);
		void CopySelectedRunOutput();
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

		enum class RunState
		{
			Queued,
			Running,
			Done,
			Failed,
		};

		enum class LauncherIcon
		{
			Setup,
			Build,
			Cook,
			Run,
			Maintain,
			Queued,
			Running,
			Done,
			Failed,
			Copy,
		};

		QWidget* CreateWorkflowSurface();
		QWidget* CreateProcessPicker(QWidget* parent);
		QPushButton* CreateProcessButton(const QString& label, const QString& operationId, QWidget* parent);
		QWidget* CreateOptionsPanel(QWidget* parent);
		QWidget* CreateOptionsPage(const QString& operationId, QWidget* parent);
		QWidget* CreateOutputPanel();
		QLabel* CreateSectionLabel(const QString& title) const;
		QLabel* CreateFieldLabel(const QString& title) const;
		QCheckBox* CreateBoundCheckBox(const QString& label, const QString& tooltip, bool checked, void (LauncherSettings::*setter)(bool));
		QLineEdit* CreateBoundLineEdit(const QString& text, const QString& placeholder, const QString& tooltip, void (LauncherSettings::*setter)(const QString&));
		QTextEdit* CreateBoundTextEdit(const QString& text, const QString& placeholder, const QString& tooltip, void (LauncherSettings::*setter)(const QString&));
		QComboBox* CreateProfileCombo(const QStringList& profiles, const QString& currentProfile, void (LauncherSettings::*setter)(const QString&));
		QComboBox* CreateProjectCombo();
		QComboBox* CreateValueCombo(const QVector<QPair<QString, QString>>& options, const QString& currentValue, void (LauncherSettings::*setter)(const QString&));
		void AddOptionsForOperation(QVBoxLayout& layout, const QString& operationId);
		QWidget* AddOptionField(QVBoxLayout& layout, const QString& label, QWidget* control);
		QWidget* AddOptionCheckBox(QVBoxLayout& layout, QCheckBox* checkBox);
		QVBoxLayout* AddOptionGroup(QVBoxLayout& layout, const QString& title, const QString& detail);
		void AddStatusRow(QVBoxLayout& layout, const QString& label, const QString& status, const QString& detail, const QString& state);
		void AddActionRow(QVBoxLayout& layout, const QString& title, const QString& detail, const QString& buttonText, const QString& operationId);
		void AddBuildEnvironmentStatus(QVBoxLayout& layout, const QString& operationId);
		QVBoxLayout* AddInlineOptionsSection(QVBoxLayout& layout);
		void AddNoOptionsMessage(QVBoxLayout& layout, const QString& text);
		void SetControlsEnabled(bool enabled);
		void LoadLauncherIconFont();
		QIcon CreateApplicationIcon() const;
		QString IconGlyph(LauncherIcon icon) const;
		QIcon CreateLauncherIcon(LauncherIcon icon, const QColor& color) const;
		QIcon WorkflowIconForIndex(int workflowIndex) const;
		QIcon ActivityIconForState(RunState state) const;
		void RegisterFocusable(QWidget* widget);
		void ConfigureTabOrder();
		void UpdateRunAvailability();
		const LauncherOperationDescriptor* FindOperationDescriptor(const QString& operationId) const;
		QString DisplayNameForOperation(const QString& operationId) const;
		QString DescriptionForOperation(const QString& operationId) const;
		bool OperationNeedsProject(const QString& operationId) const;
		bool OperationNeedsConfirmation(const QString& operationId) const;
		QString FailureRecoveryHint(const QString& operationId, const QString& statusText) const;
		LauncherOperationRequest BuildOperationRequest(const QString& operationId) const;
		bool ConfirmRunRequest(const LauncherOperationRequest& request) const;
		bool OfferLaunchPrerequisiteOperation(const QString& operationId);
		void RunInlineActionOperation(const QString& operationId);
		void StartOperation(LauncherOperationRequest request, const QString& title);
		void SetStatusMessage(const QString& message);
		void SetSelectedOperation(const QString& operationId);
		void RegisterRun(const QString& runId, const QString& title);
		void SetRunState(const QString& runId, RunState state, const QString& title);
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
		QString m_iconFontFamily;
		QButtonGroup* m_workflowGroupButtonGroup = nullptr;
		QButtonGroup* m_processButtonGroup = nullptr;
		QStackedWidget* m_operationStack = nullptr;
		QHash<QString, int> m_workflowPageByOperation;
		QHash<int, QString> m_lastOperationByWorkflowIndex;
		QVector<QWidget*> m_tabOrderWidgets;
		QStackedWidget* m_optionsStack = nullptr;
		QHash<QString, int> m_optionsPageByOperation;
		QVector<QComboBox*> m_projectSelectors;
		QTextEdit* m_operationOutput = nullptr;
		QPushButton* m_runButton = nullptr;
		QLabel* m_activeOperationLabel = nullptr;
		QLabel* m_activeOperationDescription = nullptr;
		QProgressBar* m_progressBar = nullptr;
		QLabel* m_progressLabel = nullptr;
		QWidget* m_activityDetailsPanel = nullptr;
		QListWidget* m_activityList = nullptr;
		QLabel* m_selectedRunSummary = nullptr;
		QPushButton* m_copyOutputButton = nullptr;
		QHash<QString, QListWidgetItem*> m_runItems;
		QHash<QString, RunState> m_runStates;
		QHash<QString, QString> m_runTitles;
		QHash<QString, QString> m_runOutputs;
		QString m_activeRunId;
		QString m_selectedOperationId;
		int m_nextRunIndex = 0;
		int m_startedRunCount = 0;
		int m_finishedRunCount = 0;
		int m_failedRunCount = 0;
	};
}