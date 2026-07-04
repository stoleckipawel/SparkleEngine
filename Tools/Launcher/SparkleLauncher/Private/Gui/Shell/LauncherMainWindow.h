#pragma once

#include "LauncherActionHistoryModel.h"
#include "LauncherBackend.h"
#include "LauncherCleanUiModel.h"
#include "LauncherIconLibrary.h"
#include "LauncherWorkflowCatalog.h"

#include <QtCore/QString>
#include <QtCore/QVector>
#include <QtCore/QHash>
#include <QtGui/QColor>
#include <QtGui/QIcon>
#include <QtCore/QStringList>
#include <QtWidgets/QAbstractButton>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QFrame>
#include <QtWidgets/QLabel>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QStackedWidget>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QVBoxLayout>

#include <filesystem>

namespace SparkleLauncher
{
	struct LauncherOperationDescriptor;
	class LauncherProjectModel;
	class LauncherSettings;
	struct DependencyGroupUiEntry;
	struct ThirdPartyDependencyUiEntry;
	enum class LauncherArtworkPreset;

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
		void ToggleActivityLogPanel();
		void HandleApplicationStateChanged(Qt::ApplicationState state);
		void RunSelectedOperation();
		void CleanSelectedOperation();
		void DisplayOperationStarted(const QString& runId, const QString& operationId, const QString& title);
		void AppendOperationOutput(const QString& runId, const QString& operationId, const QString& outputText);
		void DisplayOperationFinished(const QString& runId, const QString& operationId, const QString& title, const QString& statusText, int exitCode);

	private:
		struct PendingFollowUpOperation
		{
			LauncherOperationRequest Request;
			QString Title;
		};

		struct ActivityRunWidgets
		{
			QWidget* Root = nullptr;
			QFrame* Indicator = nullptr;
			QLabel* TitleLabel = nullptr;
			QLabel* StateLabel = nullptr;
		};

		enum class RunState
		{
			Queued,
			Running,
			Done,
			Failed,
		};

		QWidget* CreateWorkflowSurface();
		QWidget* CreateProcessPicker(QWidget* parent);
		QPushButton* CreateProcessButton(const QString& label, const QString& operationId, QWidget* parent);
		QWidget* CreateOptionsPanel(QWidget* parent);
		QWidget* CreateOptionsPage(const QString& operationId, QWidget* parent);
		QWidget* CreateOutputPanel();
		QWidget* CreateHeaderContextPanel(QWidget* parent);
		QLabel* CreateSectionLabel(const QString& title) const;
		QLabel* CreateFieldLabel(const QString& title) const;
		QCheckBox* CreateBoundCheckBox(const QString& label, const QString& tooltip, bool checked, void (LauncherSettings::*setter)(bool));
		QLineEdit* CreateBoundLineEdit(const QString& text, const QString& placeholder, const QString& tooltip, void (LauncherSettings::*setter)(const QString&));
		QTextEdit* CreateBoundTextEdit(const QString& text, const QString& placeholder, const QString& tooltip, void (LauncherSettings::*setter)(const QString&));
		QComboBox* CreateProfileCombo(const QStringList& profiles, const QString& currentProfile, void (LauncherSettings::*setter)(const QString&));
		QComboBox* CreateValueCombo(const QVector<QPair<QString, QString>>& options, const QString& currentValue, void (LauncherSettings::*setter)(const QString&));
		void AddOptionsForOperation(QVBoxLayout& layout, const QString& operationId);
		QWidget* AddOptionField(QVBoxLayout& layout, const QString& label, QWidget* control);
		QWidget* AddOptionCheckBox(QVBoxLayout& layout, QCheckBox* checkBox);
		QVBoxLayout* AddOptionGroup(QVBoxLayout& layout, const QString& title, const QString& detail);
		QVBoxLayout* AddDetailsGroup(QVBoxLayout& layout, const QString& title, const QString& detail, bool expanded = false);
		void AddStatusRow(QVBoxLayout& layout, const QString& label, const QString& status, const QString& detail, const QString& state, QWidget* accessory = nullptr);
		void AddSyncDependencyBundles(QVBoxLayout& layout, bool includeDependencyDetails);
		void AddSyncLevelContentGroups(QVBoxLayout& layout);
		QComboBox* CreateStartupLevelCombo();
		void PopulateStartupLevelCombo(QComboBox& combo);
		void PopulateStartupLevelSelectors();
		QVector<QPair<QString, QString>> BuildStartupLevelOptions() const;
		QString ResolveStartupLevelDisplayName() const;
		void AddWorkflowVisualBanner(QVBoxLayout& layout, const QString& operationId);
		QPushButton* CreateCommandActionButton(const QString& operationId, const QString& label, bool primary, bool runImmediately = false);
		void AddWorkflowPageHeader(QVBoxLayout& layout, const QString& operationId);
		void AddHomeQuickStart(QVBoxLayout& layout);
		void AddBuildEnvironmentStatus(QVBoxLayout& layout, const QString& operationId);
		void AddLaunchEnvironmentStatus(QVBoxLayout& layout, const QString& operationId);
		void AddLaunchTargetOptions(QVBoxLayout& layout, const QString& title, const QString& detail);
		void AddLaunchApplicationOptions(QVBoxLayout& layout);
		void AddMaintenanceEnvironmentStatus(QVBoxLayout& layout, const QString& operationId);
		QVBoxLayout* AddInlineOptionsSection(QVBoxLayout& layout);
		void AddNoOptionsMessage(QVBoxLayout& layout, const QString& text);
		void SetControlsEnabled(bool enabled);
		void EnsureOptionsPage(const QString& operationId);
		void RebuildOptionsPages();
		void ScheduleUiRefresh(bool refreshProjects);
		void ApplyScheduledUiRefresh();
		QIcon WorkflowIconForKey(const QString& iconKey) const;
		QIcon ActivityIconForState(RunState state) const;
		void RegisterFocusable(QWidget* widget);
		void SetActiveWorkflowGroup(int workflowIndex);
		void ConfigureTabOrder();
		void UpdateRunAvailability();
		QWidget* CreateTrackedDependencyActions(const ThirdPartyDependencyUiEntry& dependency);
		QWidget* CreateDisabledSourceTierActions(const DependencyGroupUiEntry& group);
		QWidget* CreateActionDependencyActions(
		    const QString& actionId,
		    const QString& actionTitle,
		    const QString& cleanScope = QString(),
		    const QString& cleanTitle = QString(),
		    bool navigateInsteadOfRun = false);
		void OpenLocalPath(const std::filesystem::path& path);
		void TriggerActionDependencyClean(const QString& cleanScope, const QString& cleanTitle);
		void TriggerActionDependencyRegenerate(const QString& actionId, const QString& actionTitle, bool navigateInsteadOfRun);
		void TriggerDependencyClean(const ThirdPartyDependencyUiEntry& dependency);
		void TriggerDependencyRegenerate(const ThirdPartyDependencyUiEntry& dependency);
		const LauncherOperationDescriptor* FindOperationDescriptor(const QString& operationId) const;
		QString DisplayNameForOperation(const QString& operationId) const;
		bool OperationNeedsProject(const QString& operationId) const;
		bool OperationNeedsConfirmation(const QString& operationId) const;
		QString FailureRecoveryHint(const QString& operationId, const QString& statusText) const;
		bool ConfirmRunRequest(LauncherOperationRequest& request) const;
		void PromptForLauncherRestart();
		bool OfferWorkspacePrerequisiteOperation(const QString& operationId);
		bool OfferCookPrerequisiteOperation(const QString& operationId);
		bool OfferLaunchPrerequisiteOperation(const QString& operationId);
		void StartOperation(LauncherOperationRequest request, const QString& title);
		void SetSelectedOperation(const QString& operationId);
		void RegisterRun(const QString& runId, const QString& title);
		void SetRunState(const QString& runId, RunState state, const QString& title);
		void AppendRunOutput(const QString& runId, const QString& text);
		void ShowRunOutput(const QString& runId);
		void SetActivityLogExpanded(bool expanded);
		void UpdateActivityRunSelectionVisuals();
		void UpdateProgress();
		void ApplyVisualStyle();

		std::filesystem::path m_repositoryRoot;
		LauncherProjectModel& m_projectModel;
		LauncherSettings& m_settings;
		LauncherBackend& m_backend;
		LauncherIconLibrary m_icons;
		QButtonGroup* m_workflowGroupButtonGroup = nullptr;
		QButtonGroup* m_processButtonGroup = nullptr;
		QStackedWidget* m_operationStack = nullptr;
		QHash<QString, int> m_workflowPageByOperation;
		QHash<int, QString> m_lastOperationByWorkflowIndex;
		QVector<QWidget*> m_tabOrderWidgets;
		QWidget* m_headerContextPanel = nullptr;
		QStackedWidget* m_optionsStack = nullptr;
		QHash<QString, int> m_optionsPageByOperation;
		QVector<QComboBox*> m_startupLevelSelectors;
		QTextEdit* m_operationOutput = nullptr;
		QPushButton* m_cleanButton = nullptr;
		QPushButton* m_runButton = nullptr;
		QPushButton* m_toggleOutputButton = nullptr;
		QLabel* m_activeOperationLabel = nullptr;
		QFrame* m_actionMetaPanel = nullptr;
		QLabel* m_progressLabel = nullptr;
		QWidget* m_activityPanel = nullptr;
		QWidget* m_activityDetailsPanel = nullptr;
		QLabel* m_activityHeaderSummary = nullptr;
		QListWidget* m_activityList = nullptr;
		QLabel* m_selectedRunSummary = nullptr;
		QPushButton* m_copyOutputButton = nullptr;
		QHash<QString, QListWidgetItem*> m_runItems;
		QHash<QString, ActivityRunWidgets> m_runItemWidgets;
		QHash<QString, RunState> m_runStates;
		QHash<QString, QString> m_runTitles;
		QHash<QString, QString> m_runOutputs;
		LauncherActionHistoryModel m_actionHistory;
		QHash<QString, PendingFollowUpOperation> m_pendingFollowUpOperations;
		QString m_activeRunId;
		QString m_selectedOperationId;
		bool m_isRebuildingOptions = false;
		bool m_isApplyingUiRefresh = false;
		bool m_uiRefreshQueued = false;
		bool m_refreshProjectsRequested = false;
		qint64 m_lastActivationRefreshMs = 0;
		bool m_activityLogExpanded = false;
		int m_nextRunIndex = 0;
		int m_startedRunCount = 0;
		int m_finishedRunCount = 0;
		int m_failedRunCount = 0;
		QStringList m_pendingRestartRunIds;
	};
}
