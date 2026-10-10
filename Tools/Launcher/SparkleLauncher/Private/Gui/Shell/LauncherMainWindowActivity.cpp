#include "LauncherMainWindow.h"

#include "LauncherActivityPanel.h"
#include "Models/LauncherOperationResult.h"

namespace SparkleLauncher
{
	void LauncherMainWindow::DisplayOperationStarted(const QString& runId, const QString&, const QString& title)
	{
		m_activityPanel->DisplayOperationStarted(runId, title);
	}

	void LauncherMainWindow::AppendOperationOutput(const QString& runId, const QString&, const QString& outputText)
	{
		m_activityPanel->AppendOperationOutput(runId, outputText);
	}

	void LauncherMainWindow::UpdateOperationProgress(const QString& runId, const QString&, const QString& phase, quint64 completed, quint64 total)
	{
		m_activityPanel->UpdateOperationProgress(runId, phase, completed, total);
	}

	void LauncherMainWindow::DisplayOperationFinished(const QString& runId, const QString& operationId, const QString& title, const LauncherOperationResult& result)
	{
		const bool succeeded = result.Succeeded;
		const QString effectiveTitle = m_activityPanel->DisplayOperationFinished(runId, title, result);

		bool refreshesSourceDependencyState = false;
		for (auto dependencyRun = m_sourceDependencyRunIds.begin(); dependencyRun != m_sourceDependencyRunIds.end();)
		{
			if (dependencyRun.value() == runId)
			{
				refreshesSourceDependencyState = true;
				dependencyRun = m_sourceDependencyRunIds.erase(dependencyRun);
			}
			else
			{
				++dependencyRun;
			}
		}
		m_cleaningSourceDependencyRunIds.remove(runId);

		const bool refreshesLevelState = operationId == QStringLiteral("levels.sync") || m_pendingLevelSelectionUpdates.contains(runId);
		if (m_pendingLevelSelectionUpdates.contains(runId))
		{
			const PendingLevelSelectionUpdate update = m_pendingLevelSelectionUpdates.take(runId);
			if (succeeded)
			{
				SetLevelsSelected(update.ContentRoot, update.LevelIds, update.Selected, effectiveTitle);
			}
		}
		if (refreshesLevelState)
		{
			RefreshLevelActionButtons();
			UpdateRunAvailability();
		}
		else if (refreshesSourceDependencyState)
		{
			RefreshSourceDependencyRows();
			UpdateRunAvailability();
		}
		else
		{
			ScheduleUiRefresh(true);
		}

		const bool launcherRestartPending = m_pendingRestartRunIds.removeAll(runId) > 0;
		if (succeeded && launcherRestartPending)
		{
			PromptForLauncherRestart();
		}

		HandleQuickStartOperationFinished(runId, operationId, succeeded, succeeded ? result.Status : result.Problem);
	}
}
