#pragma once

#include "LauncherActionHistoryModel.h"
#include "LauncherBackend.h"
#include "LauncherCleanUiModel.h"
#include "LauncherIconLibrary.h"
#include "LauncherQuickStartExecution.h"
#include "LauncherWorkflowCatalog.h"

#include <QtCore/QString>
#include <QtCore/QVector>
#include <QtCore/QHash>
#include <QtCore/QPointer>
#include <QtCore/QSet>
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
#include <optional>
#include <string>
#include <vector>

class QGridLayout;

namespace SparkleLauncher
{
	struct LauncherOperationDescriptor;
	struct ToolchainItemStatus;
	enum class WorkspaceCompiler;
	class LauncherContentModel;
	class LauncherSettings;
	struct ThirdPartyDependencyUiEntry;
	struct ThirdPartyDependencyUiStatus;
	struct LauncherLevelUiEntry;
	struct LauncherLevelUiModel;
	class ResponsiveCardGridWidget;
	struct LauncherContentSummary;
	enum class LauncherArtworkPreset;

	class LauncherMainWindow final : public QMainWindow
	{
		Q_OBJECT

	public:
		LauncherMainWindow(
		    std::filesystem::path repositoryRoot,
		    LauncherContentModel& contentModel,
		    LauncherSettings& settings,
		    LauncherBackend& backend,
		    QWidget* parent = nullptr);

		void SetStartupNotice(const QString& message);

	private slots:
		void RefreshContent();
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
		void DisplayOperationFinished(
		    const QString& runId,
		    const QString& operationId,
		    const QString& title,
		    const QString& statusText,
		    int exitCode);

	private:
		struct PendingLevelSelectionUpdate
		{
			std::filesystem::path ContentRoot;
			std::vector<std::string> LevelIds;
			bool Selected = false;
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
			Canceled,
			Failed,
		};

		QWidget* CreateWorkflowSurface();
		QWidget* CreateProcessPicker(QWidget* parent);
		QPushButton* CreateProcessButton(const QString& label, const QString& operationId, QWidget* parent);
		QWidget* CreateOptionsPanel(QWidget* parent);
		QWidget* CreateOptionsPage(const QString& operationId, QWidget* parent);
		QWidget* CreateOutputPanel();
		QWidget* CreateFooterContextPanel(QWidget* parent);
		QLabel* CreateSectionLabel(const QString& title) const;
		QLabel* CreateFieldLabel(const QString& title) const;
		QCheckBox* CreateBoundCheckBox(const QString& label, const QString& tooltip, bool checked, void (LauncherSettings::*setter)(bool));
		QLineEdit* CreateBoundLineEdit(
		    const QString& text,
		    const QString& placeholder,
		    const QString& tooltip,
		    void (LauncherSettings::*setter)(const QString&));
		QTextEdit* CreateBoundTextEdit(
		    const QString& text,
		    const QString& placeholder,
		    const QString& tooltip,
		    void (LauncherSettings::*setter)(const QString&));
		QComboBox* CreateProfileCombo(
		    const QStringList& profiles,
		    const QString& currentProfile,
		    void (LauncherSettings::*setter)(const QString&));
		QComboBox* CreateValueCombo(
		    const QVector<QPair<QString, QString>>& options,
		    const QString& currentValue,
		    void (LauncherSettings::*setter)(const QString&));
		QComboBox* CreateContextCombo(void (LauncherSettings::*setter)(const QString&));
		void RefreshContextSelectors();
		static bool UsesBuildEnvironmentStatus(const QString& operationId);
		void AddOptionsForOperation(QVBoxLayout& layout, const QString& operationId);
		void AddBuildOptions(QVBoxLayout& layout);
		void AddCookOptions(QVBoxLayout& layout);
		void AddWorkflowScopeRow(
		    QVBoxLayout& layout,
		    const QString& label,
		    const QString& value,
		    const QString& detail,
		    const QString& metadata,
		    bool available,
		    const QStringList& selectedScopes,
		    QVector<QCheckBox*>& scopeBoxes);
		void AddWorkflowAutomationNote(QVBoxLayout& layout, const QString& detail);
		void UpdateBuildScopeSetting(const QVector<QCheckBox*>& scopeBoxes, QLabel* selectionSummary);
		void UpdateCookScopeSetting(const QVector<QCheckBox*>& scopeBoxes, QLabel* selectionSummary);
		void AddCleanOptions(QVBoxLayout& layout, const QString& operationId);
		void AddCleanScopeRow(
		    QVBoxLayout& layout,
		    const CleanScopeUiOption& scope,
		    const QString& activeContentId,
		    const QStringList& selectedScopes,
		    QVector<QCheckBox*>& scopeBoxes);
		void UpdateCleanScopeSetting(const QVector<QCheckBox*>& scopeBoxes, QLabel* selectionSummary, QCheckBox* changedScope = nullptr);
		QWidget* AddOptionField(QVBoxLayout& layout, const QString& label, QWidget* control);
		QVBoxLayout* AddOptionGroup(QVBoxLayout& layout, const QString& title, const QString& detail);
		QLabel* AddStatusRow(
		    QVBoxLayout& layout,
		    const QString& label,
		    const QString& status,
		    const QString& detail,
		    const QString& state,
		    QWidget* accessory = nullptr);
		void AddSyncDependencies(QVBoxLayout& layout, bool optional);
		QPushButton* CreateSourceDependencyActionButton(const ThirdPartyDependencyUiEntry& dependency);
		void ApplySourceDependencyRowState(const ThirdPartyDependencyUiEntry& dependency, QLabel& statusLabel, QPushButton& button);
		void RefreshSourceDependencyRows();
		void SyncSourceDependency(const ThirdPartyDependencyUiEntry& dependency);
		void CleanSourceDependency(const ThirdPartyDependencyUiEntry& dependency);
		void TrackSourceDependencyRun(const LauncherOperationRequest& request, const QString& runId);
		void AddSyncLevelContentGroups(QVBoxLayout& layout);
		void AddSyncLevelRows(QVBoxLayout& layout, const LauncherContentSummary& content, const LauncherLevelUiModel& model);
		void AddSyncLevelRow(ResponsiveCardGridWidget& grid, const LauncherContentSummary& content, const LauncherLevelUiEntry& level);
		void ApplyLevelActionButtonState(QLabel& statusLabel, QPushButton& button, const LauncherLevelUiEntry& level);
		void RefreshLevelActionButtons();
		void SyncAllLevels();
		void CleanAllLevels();
		QVector<LauncherCleanTarget> BuildLevelCleanTargets(
		    const LauncherContentSummary& content,
		    const QString& levelId = QString()) const;
		bool SetLevelsSelected(
		    const std::filesystem::path& contentRoot,
		    const std::vector<std::string>& levelIds,
		    bool selected,
		    const QString& actionName);
		LauncherLevelUiModel BuildLevelUiModel() const;
		QPushButton* CreateCommandActionButton(const QString& operationId, const QString& label, bool primary, bool runImmediately = false);
		void AddHomeQuickStart(QVBoxLayout& layout);
		void StartQuickStartLevel(const LauncherContentSummary& content, const LauncherLevelUiEntry& level);
		void ContinueQuickStart();
		void HandleQuickStartOperationFinished(const QString& runId, const QString& operationId, bool succeeded, const QString& statusText);
		void ReportQuickStartBlocked(const QString& statusMessage);
		void SetQuickStartButtonsEnabled(bool enabled);
		void AddBuildEnvironmentStatus(QVBoxLayout& layout, const QString& operationId);
		void AddCookEnvironmentStatus(QVBoxLayout& layout, const QString& operationId);
		QPushButton* CreateHostToolActionButton(const ToolchainItemStatus& item);
		void SelectWorkspaceCompiler(WorkspaceCompiler compiler);
		void InstallHostTool(const ToolchainItemStatus& item);
		void AddMaintenanceEnvironmentStatus(QVBoxLayout& layout, const QString& operationId);
		QVBoxLayout* AddInlineOptionsSection(QVBoxLayout& layout);
		void AddNoOptionsMessage(QVBoxLayout& layout, const QString& text);
		void SetControlsEnabled(bool enabled);
		void EnsureOptionsPage(const QString& operationId);
		void RebuildOptionsPages();
		void ScheduleUiRefresh(bool refreshContent);
		void ApplyScheduledUiRefresh();
		QIcon WorkflowIconForPageKind(LauncherWorkflowPageKind pageKind) const;
		QIcon ActivityIconForState(RunState state) const;
		void RegisterFocusable(QWidget* widget);
		void SetActiveWorkflowGroup(int workflowIndex);
		void ConfigureTabOrder();
		void UpdateRunAvailability();
		QPushButton* CreateStatusActionButton(
		    const QString& actionId,
		    const QString& actionLabel,
		    const QString& actionTitle,
		    bool navigateInsteadOfRun = false);
		void TriggerActionDependencyRegenerate(const QString& actionId, const QString& actionTitle, bool navigateInsteadOfRun);
		const LauncherOperationDescriptor* FindOperationDescriptor(const QString& operationId) const;
		QString DisplayNameForOperation(const QString& operationId) const;
		bool OperationNeedsContent(const QString& operationId) const;
		bool OperationNeedsConfirmation(const QString& operationId) const;
		QString FailureRecoveryHint(const QString& operationId, const QString& statusText) const;
		bool ConfirmRunRequest(LauncherOperationRequest& request) const;
		void PromptForLauncherRestart();
		QString CreateRunId();
		QString StartOperation(LauncherOperationRequest request, const QString& title);
		void SetSelectedOperation(const QString& operationId);
		void RegisterRun(const QString& runId, const QString& title);
		void SetRunState(const QString& runId, RunState state, const QString& title);
		void AppendRunOutput(const QString& runId, const QString& text);
		void ShowRunOutput(const QString& runId);
		void SetActivityLogExpanded(bool expanded);
		void UpdateActivityRunSelectionVisuals();
		void RefreshActivityPanel();
		void ApplyVisualStyle();

		std::filesystem::path m_repositoryRoot;
		LauncherContentModel& m_contentModel;
		LauncherSettings& m_settings;
		LauncherBackend& m_backend;
		LauncherIconLibrary m_icons;
		QButtonGroup* m_workflowGroupButtonGroup = nullptr;
		QButtonGroup* m_processButtonGroup = nullptr;
		QStackedWidget* m_operationStack = nullptr;
		QHash<QString, int> m_workflowPageByOperation;
		QHash<int, QString> m_lastOperationByWorkflowIndex;
		QVector<QWidget*> m_tabOrderWidgets;
		QComboBox* m_runModeCombo = nullptr;
		QComboBox* m_buildConfigurationCombo = nullptr;
		QComboBox* m_workspaceCompilerCombo = nullptr;
		QComboBox* m_workspaceIdeCombo = nullptr;
		QComboBox* m_graphicsApiCombo = nullptr;
		QComboBox* m_shaderBackendCombo = nullptr;
		QStackedWidget* m_optionsStack = nullptr;
		QHash<QString, int> m_optionsPageByOperation;
		QTextEdit* m_operationOutput = nullptr;
		QPushButton* m_cleanButton = nullptr;
		QPushButton* m_runButton = nullptr;
		QPushButton* m_toggleOutputButton = nullptr;
		QFrame* m_actionMetaPanel = nullptr;
		QWidget* m_activityPanel = nullptr;
		QWidget* m_activityDetailsPanel = nullptr;
		QListWidget* m_activityList = nullptr;
		QLabel* m_selectedRunSummary = nullptr;
		QPushButton* m_copyOutputButton = nullptr;
		QHash<QString, QListWidgetItem*> m_runItems;
		QHash<QString, ActivityRunWidgets> m_runItemWidgets;
		QHash<QString, RunState> m_runStates;
		QHash<QString, QString> m_runTitles;
		QHash<QString, QString> m_runOutputs;
		LauncherActionHistoryModel m_actionHistory;
		std::optional<LauncherQuickStartExecution> m_quickStartExecution;
		QHash<QString, PendingLevelSelectionUpdate> m_pendingLevelSelectionUpdates;
		QHash<QString, QPointer<QLabel>> m_levelStatusLabels;
		QHash<QString, QPointer<QPushButton>> m_levelActionButtons;
		QHash<QString, QString> m_sourceDependencyRunIds;
		QHash<QString, QPointer<QLabel>> m_sourceDependencyStatusLabels;
		QHash<QString, QPointer<QPushButton>> m_sourceDependencyActionButtons;
		QSet<QString> m_cleaningSourceDependencyRunIds;
		QString m_activeRunId;
		QString m_selectedOperationId;
		bool m_isRebuildingOptions = false;
		bool m_isApplyingUiRefresh = false;
		bool m_uiRefreshQueued = false;
		bool m_refreshContentRequested = false;
		qint64 m_lastActivationRefreshMs = 0;
		bool m_activityLogExpanded = false;
		int m_nextRunIndex = 0;
		int m_startedRunCount = 0;
		int m_finishedRunCount = 0;
		int m_failedRunCount = 0;
		QStringList m_pendingRestartRunIds;
	};
}
