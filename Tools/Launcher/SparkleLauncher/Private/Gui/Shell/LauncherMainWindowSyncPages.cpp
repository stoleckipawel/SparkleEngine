#include "LauncherMainWindow.h"

#include "LauncherActionWidgets.h"
#include "LauncherContextUiModel.h"
#include "LauncherContentModel.h"
#include "LauncherDependencyUiModel.h"
#include "LauncherLevelUiModel.h"
#include "LauncherOperationRequestFactory.h"
#include "LauncherSelectionWidgets.h"
#include "LauncherSettings.h"

#include "SparkleLauncher/LauncherPaths.h"
#include "SparkleLauncher/MaintenanceOperations.h"
#include "SparkleLauncher/SourceDependencyState.h"

#include <QtCore/QSignalBlocker>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QVBoxLayout>

#include <filesystem>
#include <string>
#include <system_error>
#include <utility>

namespace SparkleLauncher
{
	void LauncherMainWindow::AddSyncDependencies(QVBoxLayout& layout, bool optional)
	{
		const std::filesystem::path dependencyCachePath = GetBuildDirectory(m_repositoryRoot) / "_deps";
		for (const ThirdPartyDependencyUiEntry& dependency : GetTrackedThirdPartyDependencies())
		{
			if (dependency.Required == optional)
			{
				continue;
			}

			const ThirdPartyDependencyUiStatus status = BuildThirdPartyDependencyStatus(dependency, dependencyCachePath);
			const QString runId = m_sourceDependencyRunIds.value(dependency.Id);
			const bool syncing = !runId.isEmpty() && !m_cleaningSourceDependencyRunIds.contains(runId);
			const bool cleaning = !runId.isEmpty() && m_cleaningSourceDependencyRunIds.contains(runId);
			QPushButton* actionButton = CreateSourceDependencyActionButton(dependency);
			QLabel* statusLabel = AddStatusRow(
			    layout,
			    dependency.Label,
			    cleaning      ? QStringLiteral("Cleaning")
			        : syncing ? QStringLiteral("Syncing")
			                  : status.Text,
			    status.Detail,
			    cleaning || syncing ? QStringLiteral("running") : status.State,
			    actionButton);
			ApplySourceDependencyRowState(dependency, *statusLabel, *actionButton);
			m_sourceDependencyStatusLabels.insert(dependency.Id, statusLabel);
			m_sourceDependencyActionButtons.insert(dependency.Id, actionButton);
		}
	}

	QPushButton* LauncherMainWindow::CreateSourceDependencyActionButton(const ThirdPartyDependencyUiEntry& dependency)
	{
		QPushButton* button = new QPushButton(this);
		RegisterFocusable(button);
		connect(
		    button,
		    &QPushButton::clicked,
		    this,
		    [this, dependency, button]()
		    {
			    const QString actionIntent = button->property("ActionIntent").toString();
			    if (actionIntent == QStringLiteral("clean"))
			    {
				    CleanSourceDependency(dependency);
			    }
			    else if (actionIntent == QStringLiteral("sync"))
			    {
				    SyncSourceDependency(dependency);
			    }
		    });
		return button;
	}

	void LauncherMainWindow::ApplySourceDependencyRowState(
	    const ThirdPartyDependencyUiEntry& dependency,
	    QLabel& statusLabel,
	    QPushButton& button)
	{
		const std::filesystem::path dependencyCachePath = GetBuildDirectory(m_repositoryRoot) / "_deps";
		const ThirdPartyDependencyUiStatus status = BuildThirdPartyDependencyStatus(dependency, dependencyCachePath);
		const QString runId = m_sourceDependencyRunIds.value(dependency.Id);
		const bool cleaning = !runId.isEmpty() && m_cleaningSourceDependencyRunIds.contains(runId);
		const bool syncing = !runId.isEmpty() && !cleaning;
		const SyncItemState state = syncing ? SyncItemState::Syncing : status.Synced ? SyncItemState::Synced : SyncItemState::Missing;

		ApplyInlineStatusLabel(
		    statusLabel,
		    cleaning      ? QStringLiteral("Cleaning")
		        : syncing ? QStringLiteral("Syncing")
		                  : status.Text,
		    cleaning || syncing ? QStringLiteral("running") : status.State);
		ApplySyncActionButtonState(button, state, dependency.Label);
		if (cleaning)
		{
			button.setText("Cleaning...");
			button.setProperty("ActionIntent", "none");
			button.setEnabled(false);
			button.setAccessibleName("Cleaning " + dependency.Label);
			button.setToolTip("Clean is in progress.");
		}
	}

	void LauncherMainWindow::RefreshSourceDependencyRows()
	{
		for (const ThirdPartyDependencyUiEntry& dependency : GetTrackedThirdPartyDependencies())
		{
			QLabel* statusLabel = m_sourceDependencyStatusLabels.value(dependency.Id, nullptr);
			QPushButton* actionButton = m_sourceDependencyActionButtons.value(dependency.Id, nullptr);
			if (statusLabel != nullptr && actionButton != nullptr)
			{
				ApplySourceDependencyRowState(dependency, *statusLabel, *actionButton);
			}
		}
	}

	void LauncherMainWindow::SyncSourceDependency(const ThirdPartyDependencyUiEntry& dependency)
	{
		LauncherOperationRequest request =
		    BuildLauncherOperationRequest(m_repositoryRoot, m_contentModel, m_settings, "workspace.sync-code");
		request.SourceDependencyId = dependency.Id;
		StartOperation(std::move(request), "Sync " + dependency.Label);
	}

	void LauncherMainWindow::CleanSourceDependency(const ThirdPartyDependencyUiEntry& dependency)
	{
		const SourceDependencyEntry* sourceDependency = FindSourceDependency(dependency.Id.toStdString());
		if (sourceDependency == nullptr)
		{
			return;
		}

		LauncherOperationRequest request = BuildLauncherOperationRequest(m_repositoryRoot, m_contentModel, m_settings, "workspace.clean");
		const std::filesystem::path dependencyCachePath = GetBuildDirectory(m_repositoryRoot) / "_deps";
		for (const std::filesystem::path& path : GetSourceDependencyCachePaths(*sourceDependency, dependencyCachePath))
		{
			std::error_code errorCode;
			if (!std::filesystem::exists(path, errorCode) || errorCode)
			{
				continue;
			}
			request.CleanTargets.push_back(
			    {dependency.Label + " cache", QString::fromStdString(path.string()), "Generated source dependency cache."});
		}
		if (request.CleanTargets.empty() || !ConfirmRunRequest(request))
		{
			return;
		}

		const QString runId = StartOperation(std::move(request), "Clean " + dependency.Label);
		m_sourceDependencyRunIds.insert(dependency.Id, runId);
		m_cleaningSourceDependencyRunIds.insert(runId);
		RefreshSourceDependencyRows();
	}

	void LauncherMainWindow::TrackSourceDependencyRun(const LauncherOperationRequest& request, const QString& runId)
	{
		if (request.OperationId != "workspace.sync-code")
		{
			return;
		}

		if (!request.SourceDependencyId.isEmpty())
		{
			m_sourceDependencyRunIds.insert(request.SourceDependencyId, runId);
			RefreshSourceDependencyRows();
			return;
		}

		for (const ThirdPartyDependencyUiEntry& dependency : GetTrackedThirdPartyDependencies())
		{
			if (!dependency.Enabled)
			{
				continue;
			}
			m_sourceDependencyRunIds.insert(dependency.Id, runId);
		}
		RefreshSourceDependencyRows();
	}

	QComboBox* LauncherMainWindow::CreateStartupLevelCombo()
	{
		QComboBox* combo = new QComboBox(this);
		combo->setProperty("StartupLevelSelector", true);
		combo->setToolTip("Startup level used by editor and runtime launches.");
		combo->setAccessibleName("Startup level");
		combo->setAccessibleDescription("Startup level used by editor and runtime launches.");
		RegisterFocusable(combo);
		m_startupLevelSelectors.push_back(combo);
		PopulateStartupLevelCombo(*combo);
		connect(
		    combo,
		    static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
		    [combo, this]()
		    {
			    const QString levelId = combo->currentData().toString();
			    if (!levelId.isEmpty())
			    {
				    m_settings.SetStartupLevel(levelId);
			    }
		    });
		return combo;
	}

	void LauncherMainWindow::PopulateStartupLevelCombo(QComboBox& combo)
	{
		const LauncherLevelUiModel model = BuildLevelUiModel();
		if (!model.Loaded)
		{
			const QSignalBlocker blocker(&combo);
			combo.clear();
			combo.addItem(QStringLiteral("Level catalog unavailable"), QString());
			combo.setEnabled(false);
			combo.setToolTip(model.LoadError.isEmpty() ? QStringLiteral("The level catalog could not be loaded.") : model.LoadError);
			return;
		}

		const QVector<LauncherStartupLevelUiEntry>& options = model.StartupLevels;
		if (options.empty())
		{
			const QSignalBlocker blocker(&combo);
			combo.clear();
			combo.addItem(QStringLiteral("No catalog levels"), QString());
			combo.setEnabled(false);
			combo.setToolTip("No catalog levels are available.");
			return;
		}

		QVector<LauncherSelectionOption> selectionOptions;
		selectionOptions.reserve(options.size() + 1);
		bool hasReadyLevel = false;
		for (const LauncherStartupLevelUiEntry& option : options)
		{
			hasReadyLevel = hasReadyLevel || option.Ready;
			selectionOptions.push_back(
			    {option.DisplayName,
			        option.Id,
			        option.Ready ? QStringLiteral("Synced and ready to launch.")
			                     : QStringLiteral("%1. Open Sync > Sync Levels to make this level available.").arg(option.Status),
			        option.Ready});
		}
		if (!hasReadyLevel)
		{
			selectionOptions.push_front(
			    {QStringLiteral("Built-in Empty"), QString(), QStringLiteral("Always available when no catalog level is ready."), true});
		}

		PopulateLauncherSelectionCombo(combo, selectionOptions, m_settings.StartupLevel());
		combo.setToolTip(
		    "Startup level used by editor and runtime launches. Synced levels are selectable; supported unsynced levels remain visible for "
		    "setup.");
		if (combo.currentIndex() >= 0)
		{
			const QString effectiveLevelId = combo.currentData().toString();
			if (m_settings.StartupLevel() != effectiveLevelId)
			{
				m_settings.SetStartupLevel(effectiveLevelId);
			}
		}
	}

	void LauncherMainWindow::PopulateStartupLevelSelectors()
	{
		for (QComboBox* combo : m_startupLevelSelectors)
		{
			if (combo != nullptr)
			{
				PopulateStartupLevelCombo(*combo);
			}
		}
	}

	QVector<QPair<QString, QString>> LauncherMainWindow::BuildStartupLevelOptions() const
	{
		QVector<QPair<QString, QString>> options;
		const LauncherLevelUiModel model = BuildLevelUiModel();
		for (const LauncherStartupLevelUiEntry& option : model.StartupLevels)
		{
			if (option.Ready)
			{
				options.push_back({option.DisplayName, option.Id});
			}
		}
		return options;
	}

	LauncherLevelUiModel LauncherMainWindow::BuildLevelUiModel() const
	{
		const LauncherContentSummary* content = m_contentModel.Content();
		if (content == nullptr)
		{
			return {};
		}

		return LauncherLevelUiModel::Build(*content);
	}

	QString LauncherMainWindow::ResolveStartupLevelDisplayName() const
	{
		const QVector<QPair<QString, QString>> options = BuildStartupLevelOptions();
		const QString selectedLevel = m_settings.StartupLevel();
		for (const QPair<QString, QString>& option : options)
		{
			if (!selectedLevel.isEmpty() && option.second == selectedLevel)
			{
				return option.first;
			}
		}
		if (!selectedLevel.isEmpty())
		{
			return selectedLevel;
		}
		return options.empty() ? QStringLiteral("Built-in Empty") : options.front().first;
	}
}
