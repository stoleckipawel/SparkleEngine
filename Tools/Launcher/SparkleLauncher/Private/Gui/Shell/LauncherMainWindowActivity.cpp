#include "LauncherMainWindow.h"

#include "LauncherActivityPanel.h"

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

	void LauncherMainWindow::DisplayOperationFinished(
	    const QString& runId,
	    const QString& operationId,
	    const QString& title,
	    const QString& statusText,
	    int exitCode)
	{
		const bool succeeded = exitCode == 0;
		const QString effectiveTitle =
		    m_activityPanel->DisplayOperationFinished(runId, title, statusText, exitCode, FailureRecoveryHint(operationId, statusText));

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

		HandleQuickStartOperationFinished(runId, operationId, succeeded, statusText);
	}
}
