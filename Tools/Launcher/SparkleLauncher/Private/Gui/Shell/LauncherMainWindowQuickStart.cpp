#include "LauncherMainWindow.h"

#include "LauncherContentModel.h"
#include "LauncherLevelUiModel.h"
#include "LauncherOperationRequestFactory.h"
#include "LauncherQuickStartPlanner.h"
#include "LauncherSettings.h"

#include <QtCore/QDateTime>
#include <QtCore/QStringList>
#include <QtWidgets/QPushButton>

#include <string>
#include <utility>

namespace SparkleLauncher
{
	static QString QuickStartGoalDisplayName(const LauncherOperationRequest& request)
	{
		const QString levelId = request.RequestedLevelIds.section(',', 0, 0).trimmed();
		return levelId.isEmpty() ? QStringLiteral("level") : levelId;
	}

	void LauncherMainWindow::StartQuickStartLevel(const LauncherContentSummary& content, const LauncherLevelUiEntry& level)
	{
		if (m_quickStartExecution.has_value())
		{
			if (!m_quickStartExecution->ActiveRunId().isEmpty())
			{
				AppendRunOutput(m_quickStartExecution->ActiveRunId(), "Quick Start is already preparing a level.\n");
				ShowRunOutput(m_quickStartExecution->ActiveRunId());
			}
			return;
		}

		if (!level.RuntimeSupported || !level.CanSelect)
		{
			return;
		}
		const QString actionName = m_settings.RunMode() == QStringLiteral("game") ? QStringLiteral("Run ") : QStringLiteral("Open ");
		if (!SetLevelsSelected(content.RootPath, {level.Id.toStdString()}, true, actionName + level.DisplayName))
		{
			return;
		}

		LauncherOperationRequest goalRequest =
		    BuildLauncherOperationRequest(m_repositoryRoot, m_contentModel, m_settings, QStringLiteral("levels.run"));
		goalRequest.ContentId = content.Id;
		goalRequest.RequestedLevelIds = level.Id;
		m_quickStartExecution.emplace(std::move(goalRequest));
		SetQuickStartButtonsEnabled(false);
		ContinueQuickStart();
	}

	void LauncherMainWindow::ContinueQuickStart()
	{
		if (!m_quickStartExecution.has_value())
		{
			return;
		}

		LauncherQuickStartExecution& execution = *m_quickStartExecution;
		const LauncherLevelUiModel levelModel = BuildLevelUiModel();
		LauncherCapabilityResolution resolution =
		    PlanLauncherQuickStartStep(execution.GoalRequest(), levelModel, execution.InvalidatedCapabilityIds());
		if (resolution.Result == LauncherCapabilityResolution::Kind::Blocked)
		{
			ReportQuickStartBlocked(QString::fromStdString(resolution.StatusMessage));
			return;
		}
		if (resolution.Result == LauncherCapabilityResolution::Kind::Ready)
		{
			ReportQuickStartBlocked("The requested level became ready without registering its final run operation.");
			return;
		}
		if (!resolution.OperationRequest.has_value())
		{
			ReportQuickStartBlocked("The prerequisite graph selected an operation without a request.");
			return;
		}

		const QString operationId = resolution.OperationRequest->OperationId;
		if (FindOperationDescriptor(operationId) == nullptr)
		{
			ReportQuickStartBlocked(QStringLiteral("Capability %1 selected unknown launcher operation %2.")
			        .arg(QString::fromStdString(resolution.CapabilityId), operationId));
			return;
		}

		const QString goalName = QuickStartGoalDisplayName(execution.GoalRequest());
		const QString launchVerb =
		    execution.GoalRequest().RunMode == QStringLiteral("game") ? QStringLiteral("Run") : QStringLiteral("Open");
		const QString stepTitle = QStringLiteral("%1 %2 - %3").arg(launchVerb, goalName, DisplayNameForOperation(operationId));
		QStringList dependencyPath;
		for (const std::string& capabilityId : resolution.DependencyPath)
		{
			dependencyPath.push_back(QString::fromStdString(capabilityId));
		}
		QStringList invalidatedCapabilities;
		for (const std::string& capabilityId : resolution.InvalidatedCapabilityIds)
		{
			invalidatedCapabilities.push_back(QString::fromStdString(capabilityId));
		}

		const QString runId = CreateRunId();
		resolution.OperationRequest->RunId = runId;
		const std::string beginError = execution.BeginOperation(runId, resolution);
		if (!beginError.empty())
		{
			ReportQuickStartBlocked(QString::fromStdString(beginError));
			return;
		}

		StartOperation(std::move(*resolution.OperationRequest), stepTitle);
		AppendRunOutput(
		    runId,
		    QStringLiteral("Quick Start is preparing every registered prerequisite needed to run %1.\nCapability path: %2\nOperation: %3\n")
		        .arg(goalName, dependencyPath.join(" -> "), operationId));
		if (!invalidatedCapabilities.isEmpty())
		{
			AppendRunOutput(runId, QStringLiteral("Invalidates after success: %1\n").arg(invalidatedCapabilities.join(", ")));
		}
		ShowRunOutput(runId);
	}

	void LauncherMainWindow::HandleQuickStartOperationFinished(
	    const QString& runId,
	    const QString& operationId,
	    bool succeeded,
	    const QString& statusText)
	{
		if (!m_quickStartExecution.has_value())
		{
			return;
		}

		const LauncherQuickStartCompletion completion = m_quickStartExecution->CompleteOperation(runId, operationId, succeeded);
		switch (completion)
		{
			case LauncherQuickStartCompletion::Ignored:
				return;
			case LauncherQuickStartCompletion::Failed:
				AppendRunOutput(runId, QStringLiteral("\nQuick Start stopped because this prerequisite failed: %1\n").arg(statusText));
				ShowRunOutput(runId);
				m_quickStartExecution.reset();
				SetQuickStartButtonsEnabled(true);
				ScheduleUiRefresh(true);
				return;
			case LauncherQuickStartCompletion::Completed:
				AppendRunOutput(runId, "\nQuick Start completed. The requested level was started.\n");
				ShowRunOutput(runId);
				m_quickStartExecution.reset();
				SetQuickStartButtonsEnabled(true);
				ScheduleUiRefresh(true);
				return;
			case LauncherQuickStartCompletion::Continue:
				ContinueQuickStart();
				return;
		}
	}

	void LauncherMainWindow::ReportQuickStartBlocked(const QString& statusMessage)
	{
		if (!m_quickStartExecution.has_value())
		{
			return;
		}

		const LauncherOperationRequest goalRequest = m_quickStartExecution->GoalRequest();
		const QString goalName = QuickStartGoalDisplayName(goalRequest);
		const QString title = QStringLiteral("Run %1").arg(goalName);
		const QString message = statusMessage.trimmed().isEmpty() ? QStringLiteral("Quick Start could not resolve the next prerequisite.")
		                                                          : statusMessage.trimmed();
		m_quickStartExecution.reset();
		SetQuickStartButtonsEnabled(true);

		const QString runId = CreateRunId();
		RegisterRun(runId, title);
		SetRunState(runId, RunState::Failed, title);
		AppendRunOutput(runId, QStringLiteral("Quick Start is blocked.\n\n%1\n").arg(message));
		++m_finishedRunCount;
		++m_failedRunCount;
		m_actionHistory.RecordCompletion(goalRequest.OperationId, QDateTime::currentDateTimeUtc().toString(Qt::ISODate), message, 1);
		m_actionHistory.Save(m_repositoryRoot);
		ShowRunOutput(runId);
		SetActivityLogExpanded(true);
		RefreshActivityPanel();
		ScheduleUiRefresh(true);
	}

	void LauncherMainWindow::SetQuickStartButtonsEnabled(bool enabled)
	{
		for (const QPointer<QPushButton>& button : m_levelActionButtons)
		{
			if (button != nullptr)
			{
				button->setEnabled(enabled);
			}
		}
	}
}
