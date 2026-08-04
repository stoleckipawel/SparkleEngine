#include "LauncherMainWindow.h"

#include "LauncherLevelUiModel.h"
#include "LauncherOperationRequestFactory.h"
#include "LauncherQuickStartPlanner.h"
#include "LauncherUiDesign.h"

#include <QtCore/QDateTime>
#include <QtCore/QStringList>
#include <QtWidgets/QPushButton>

#include <string>
#include <utility>

namespace SparkleLauncher
{
	static QString QuickStartProductDisplayName(const LauncherOperationRequest& request)
	{
		return request.LaunchTarget == "runtime" ? QStringLiteral("Runtime") : QStringLiteral("Editor");
	}

	QPushButton* LauncherMainWindow::CreateQuickStartButton(const QString& launchOperationId, const QString& label)
	{
		QPushButton* button = new QPushButton(label, this);
		button->setObjectName("CommandPrimaryButton");
		button->setMinimumHeight(LauncherUi::Button::PrimaryMinHeight);
		button->setAccessibleName(label);
		button->setToolTip("Automatically prepare every available prerequisite, then launch.");
		button->setEnabled(!m_quickStartExecution.has_value());
		RegisterFocusable(button);
		m_quickStartButtons.push_back(button);
		connect(button, &QPushButton::clicked, this, [this, launchOperationId]() { StartQuickStart(launchOperationId); });
		return button;
	}

	void LauncherMainWindow::StartQuickStart(const QString& launchOperationId)
	{
		if (m_quickStartExecution.has_value())
		{
			if (!m_quickStartExecution->ActiveRunId().isEmpty())
			{
				AppendRunOutput(m_quickStartExecution->ActiveRunId(), "Quick Start is already preparing a product.\n");
				ShowRunOutput(m_quickStartExecution->ActiveRunId());
			}
			return;
		}

		if (launchOperationId != "launch.editor" && launchOperationId != "launch.runtime")
		{
			return;
		}

		m_quickStartExecution.emplace(BuildLauncherOperationRequest(m_repositoryRoot, m_contentModel, m_settings, launchOperationId));
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
			ReportQuickStartBlocked("The Quick Start goal became ready without registering a launch operation.");
			return;
		}
		if (!resolution.OperationRequest.has_value())
		{
			ReportQuickStartBlocked("The capability graph selected an operation without a request.");
			return;
		}

		const QString operationId = resolution.OperationRequest->OperationId;
		if (FindOperationDescriptor(operationId) == nullptr)
		{
			ReportQuickStartBlocked(QStringLiteral("Capability %1 selected unknown launcher operation %2.")
			        .arg(QString::fromStdString(resolution.CapabilityId), operationId));
			return;
		}

		const QString productName = QuickStartProductDisplayName(execution.GoalRequest());
		const QString stepTitle = QStringLiteral("Quick Start %1 - %2").arg(productName, DisplayNameForOperation(operationId));
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
		    QStringLiteral("Quick Start is preparing every registered capability needed to run %1.\nCapability path: %2\nOperation: %3\n")
		        .arg(productName, dependencyPath.join(" -> "), operationId));
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
				AppendRunOutput(runId, QStringLiteral("\nQuick Start stopped because this step failed: %1\n").arg(statusText));
				ShowRunOutput(runId);
				m_quickStartExecution.reset();
				SetQuickStartButtonsEnabled(true);
				ScheduleUiRefresh(true);
				return;
			case LauncherQuickStartCompletion::Completed:
				AppendRunOutput(runId, "\nQuick Start completed. The requested product was launched.\n");
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
		const QString productName = QuickStartProductDisplayName(goalRequest);
		const QString title = QStringLiteral("Quick Start %1").arg(productName);
		const QString message = statusMessage.trimmed().isEmpty() ? QStringLiteral("Quick Start could not resolve an automatic next step.")
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
		for (const QPointer<QPushButton>& button : m_quickStartButtons)
		{
			if (button != nullptr)
			{
				button->setEnabled(enabled);
			}
		}
	}
}
