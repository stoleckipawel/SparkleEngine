#include "LauncherMainWindow.h"

#include "LauncherActionWidgets.h"
#include "LauncherContentModel.h"
#include "LauncherDependencyUiModel.h"
#include "LauncherLevelUiModel.h"
#include "LauncherOperationRequestFactory.h"
#include "LauncherSettings.h"

#include "SparkleLauncher/LauncherPaths.h"
#include "SparkleLauncher/MaintenanceOperations.h"
#include "SparkleLauncher/SourceDependencyState.h"

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

	LauncherLevelUiModel LauncherMainWindow::BuildLevelUiModel() const
	{
		const LauncherContentSummary* content = m_contentModel.Content();
		if (content == nullptr)
		{
			return {};
		}

		return LauncherLevelUiModel::Build(*content);
	}
}
