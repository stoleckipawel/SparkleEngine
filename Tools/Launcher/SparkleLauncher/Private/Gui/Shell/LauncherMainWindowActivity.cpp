#include "LauncherMainWindow.h"

#include "LauncherActionHistoryModel.h"
#include "LauncherOutputWidgets.h"
#include "LauncherUiDesign.h"

#include <QtCore/QDateTime>
#include <QtGui/QClipboard>
#include <QtGui/QColor>
#include <QtGui/QGuiApplication>
#include <QtGui/QTextCursor>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QListWidgetItem>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QStyle>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QWidget>

namespace SparkleLauncher
{
	static constexpr int kMaxOperationOutputCharacters = 1000000;
	static constexpr int kOperationOutputMinHeight = LauncherUi::OperationOutput::MinHeight;
	static constexpr int kOperationOutputCompactMaxHeight = LauncherUi::OperationOutput::CompactMaxHeight;
	static constexpr int kOperationOutputProminentMinHeight = LauncherUi::OperationOutput::ProminentMinHeight;
	static constexpr int kOperationOutputMaxHeight = LauncherUi::OperationOutput::MaxHeight;
	static constexpr int kActivityPanelCollapsedHeight = LauncherUi::Activity::CollapsedHeight;
	static constexpr int kActivityPanelExpandedHeight = LauncherUi::Activity::ExpandedHeight;
	static constexpr const char* kColorStateQueued = LauncherUi::Color::StateQueued;
	static constexpr const char* kColorStateRunning = LauncherUi::Color::StateRunning;
	static constexpr const char* kColorStateSuccess = LauncherUi::Color::StateSuccess;
	static constexpr const char* kColorStateDestructive = LauncherUi::Color::StateDestructive;

	void LauncherMainWindow::DisplaySelectedRunOutput(QListWidgetItem* currentItem, QListWidgetItem*)
	{
		if (currentItem == nullptr)
		{
			return;
		}

		const QString runId = currentItem->data(Qt::UserRole).toString();
		ShowRunOutput(runId);
		const RunState state = m_runStates.value(runId, RunState::Done);
		if (state == RunState::Running || state == RunState::Failed)
		{
			SetActivityLogExpanded(true);
		}
	}

	void LauncherMainWindow::CopySelectedRunOutput()
	{
		if (m_operationOutput == nullptr)
		{
			return;
		}

		QGuiApplication::clipboard()->setText(m_operationOutput->toPlainText());
	}

	void LauncherMainWindow::ToggleActivityLogPanel()
	{
		SetActivityLogExpanded(!m_activityLogExpanded);
	}


	void LauncherMainWindow::DisplayOperationStarted(const QString& runId, const QString&, const QString& title)
	{
		const QString effectiveTitle = m_runTitles.value(runId, title);
		SetRunState(runId, RunState::Running, effectiveTitle);
		AppendRunOutput(runId, effectiveTitle + " started.\n");
		ShowRunOutput(runId);
		SetActivityLogExpanded(true);
		UpdateProgress();
	}

	void LauncherMainWindow::AppendOperationOutput(const QString& runId, const QString&, const QString& outputText)
	{
		AppendRunOutput(runId, outputText);
		if (m_activeRunId == runId)
		{
			ShowRunOutput(runId);
		}
	}

	void LauncherMainWindow::DisplayOperationFinished(const QString& runId, const QString& operationId, const QString& title, const QString& statusText, int exitCode)
	{
		const bool succeeded = exitCode == 0;
		const QString effectiveTitle = m_runTitles.value(runId, title);
		SetRunState(runId, succeeded ? RunState::Done : RunState::Failed, effectiveTitle);

		if (succeeded)
		{
			AppendRunOutput(runId, "\n" + effectiveTitle + " finished: " + statusText + "\n");
		}
		else
		{
			const QString existingOutput = m_runOutputs.value(runId);
			const QString recoveryHint = FailureRecoveryHint(operationId, statusText);
			const QString recoveryText = recoveryHint.isEmpty() ? QString() : QStringLiteral("Recovery: %1\n\n").arg(recoveryHint);
			m_runOutputs.insert(
			    runId,
			    QStringLiteral("Failed: %1 (exit code %2)\n").arg(statusText).arg(exitCode) + recoveryText + "\n" + existingOutput + "\n" + effectiveTitle + " finished: " + statusText + "\n");
			++m_failedRunCount;
		}

		++m_finishedRunCount;
		m_actionHistory.RecordCompletion(operationId, QDateTime::currentDateTimeUtc().toString(Qt::ISODate), statusText, exitCode);
		m_actionHistory.Save(m_repositoryRoot);
		UpdateActionHistoryDisplay();
		ShowRunOutput(runId);
		SetActivityLogExpanded(true);
		UpdateProgress();
		RefreshProjects();
		RebuildOptionsPages();

		if (succeeded && operationId == "launcher.build.self" && !m_pendingRestartRunIds.contains(runId))
		{
			m_pendingRestartRunIds.push_back(runId);
			PromptForLauncherRestart();
		}

		if (m_pendingFollowUpOperations.contains(runId))
		{
			const PendingFollowUpOperation followUp = m_pendingFollowUpOperations.take(runId);
			if (succeeded)
			{
				StartOperation(followUp.Request, followUp.Title);
			}
		}
	}


	void LauncherMainWindow::UpdateActionHistoryDisplay()
	{
		if (m_lastRunSummaryLabel == nullptr || m_lastRunResultLabel == nullptr)
		{
			return;
		}

		const LauncherActionHistoryRecord* found = m_actionHistory.Find(m_selectedOperationId);
		if (found == nullptr)
		{
			m_lastRunSummaryLabel->setText("No recorded run for this workflow yet.");
			m_lastRunResultLabel->setText("Result data will persist between launcher sessions.");
			if (m_dismissHistoryButton != nullptr)
			{
				m_dismissHistoryButton->setEnabled(false);
				m_dismissHistoryButton->setVisible(false);
			}
			return;
		}

		const QDateTime completedAt = QDateTime::fromString(found->CompletedAtUtc, Qt::ISODate);
		const QString completedAtText = completedAt.isValid() ? completedAt.toLocalTime().toString("MMMM d, yyyy HH:mm") : found->CompletedAtUtc;
		const bool failed = found->ExitCode != 0;
		m_lastRunSummaryLabel->setText(QStringLiteral("%1 on %2").arg(failed ? "Last failed" : "Last completed", completedAtText));
		if (failed)
		{
			const QString recoveryHint = FailureRecoveryHint(m_selectedOperationId, found->ResultText);
			m_lastRunResultLabel->setText(QStringLiteral("Recovery: %1").arg(recoveryHint.isEmpty() ? found->ResultText : recoveryHint));
		}
		else
		{
			m_lastRunResultLabel->setText(QStringLiteral("Result: %1").arg(found->ResultText));
		}
		if (m_dismissHistoryButton != nullptr)
		{
			m_dismissHistoryButton->setEnabled(true);
			m_dismissHistoryButton->setVisible(true);
		}
	}

	void LauncherMainWindow::DismissSelectedActionHistory()
	{
		if (m_selectedOperationId.isEmpty())
		{
			return;
		}

		if (m_actionHistory.Dismiss(m_selectedOperationId))
		{
			m_actionHistory.Save(m_repositoryRoot);
			UpdateActionHistoryDisplay();
			RebuildOptionsPages();
		}
	}


	QIcon LauncherMainWindow::ActivityIconForState(RunState state) const
	{
		switch (state)
		{
		case RunState::Queued:
			return m_icons.Icon(LauncherIcon::Queued, QColor(kColorStateQueued));
		case RunState::Running:
			return m_icons.Icon(LauncherIcon::Running, QColor(kColorStateRunning));
		case RunState::Done:
			return m_icons.Icon(LauncherIcon::Done, QColor(kColorStateSuccess));
		case RunState::Failed:
			return m_icons.Icon(LauncherIcon::Failed, QColor(kColorStateDestructive));
		}

		return {};
	}


	void LauncherMainWindow::RegisterRun(const QString& runId, const QString& title)
	{
		++m_startedRunCount;
		QListWidgetItem* item = new QListWidgetItem(m_activityList);
		item->setData(Qt::UserRole, runId);
		item->setSizeHint(QSize(0, LauncherUi::Activity::HistoryRowHeight));
		item->setText(QString());
		const LauncherActivityRowWidgets rowWidgets = CreateLauncherActivityRow(m_activityList, title);
		m_activityList->setItemWidget(item, rowWidgets.Root);
		m_runItems.insert(runId, item);
		m_runItemWidgets.insert(runId, {rowWidgets.Root, rowWidgets.Indicator, rowWidgets.TitleLabel, rowWidgets.StateLabel});
		m_runTitles.insert(runId, title);
		SetRunState(runId, RunState::Queued, title);
		m_runOutputs.insert(runId, title + " queued.\n");
		m_activityList->setCurrentItem(item);
		m_activeRunId = runId;
		UpdateProgress();
	}

	void LauncherMainWindow::SetRunState(const QString& runId, RunState state, const QString& title)
	{
		QListWidgetItem* item = m_runItems.value(runId, nullptr);
		if (item == nullptr)
		{
			return;
		}

		m_runStates.insert(runId, state);
		m_runTitles.insert(runId, title);

		QString stateText;
		QColor stateColor;
		switch (state)
		{
		case RunState::Queued:
			stateText = "Queued";
			stateColor = QColor(kColorStateQueued);
			break;
		case RunState::Running:
			stateText = "Running";
			stateColor = QColor(kColorStateRunning);
			break;
		case RunState::Done:
			stateText = "Done";
			stateColor = QColor(kColorStateSuccess);
			break;
		case RunState::Failed:
			stateText = "Failed";
			stateColor = QColor(kColorStateDestructive);
			break;
		}

		item->setText(QString());
		item->setIcon(ActivityIconForState(state));
		item->setData(Qt::UserRole + 1, stateText);
		item->setData(Qt::AccessibleTextRole, stateText + ": " + title);
		item->setData(Qt::AccessibleDescriptionRole, "Launcher activity run " + stateText.toLower());
		item->setToolTip(stateText + ": " + title);

		const ActivityRunWidgets widgets = m_runItemWidgets.value(runId);
		if (widgets.Root != nullptr)
		{
			widgets.Root->setProperty("RunState", stateText.toLower());
			if (widgets.Indicator != nullptr)
			{
				widgets.Indicator->setProperty("RunState", stateText.toLower());
				widgets.Indicator->style()->unpolish(widgets.Indicator);
				widgets.Indicator->style()->polish(widgets.Indicator);
			}
			if (widgets.TitleLabel != nullptr)
			{
				widgets.TitleLabel->setText(title);
			}
			if (widgets.StateLabel != nullptr)
			{
				widgets.StateLabel->setText(stateText);
			}
			widgets.Root->style()->unpolish(widgets.Root);
			widgets.Root->style()->polish(widgets.Root);
		}
		UpdateActivityRunSelectionVisuals();
	}

	void LauncherMainWindow::AppendRunOutput(const QString& runId, const QString& text)
	{
		QString output = m_runOutputs.value(runId);
		output += text;
		const int overflowCharacters = output.size() - kMaxOperationOutputCharacters;
		if (overflowCharacters > 0)
		{
			output.remove(0, overflowCharacters);
		}
		m_runOutputs.insert(runId, output);
	}

	void LauncherMainWindow::ShowRunOutput(const QString& runId)
	{
		m_activeRunId = runId;
		UpdateActivityRunSelectionVisuals();
		const RunState state = m_runStates.value(runId, RunState::Queued);
		const QString title = m_runTitles.value(runId, "Selected run");
		if (m_selectedRunSummary != nullptr)
		{
			switch (state)
			{
			case RunState::Queued:
				m_selectedRunSummary->setText("Queued: " + title + ". Waiting to start.");
				break;
			case RunState::Running:
				m_selectedRunSummary->setText("Running: " + title + ". Output is updating below.");
				break;
			case RunState::Done:
				m_selectedRunSummary->setText("Done: " + title + ". Output is available below.");
				break;
			case RunState::Failed:
				m_selectedRunSummary->setText("Failed: " + title + ". Review the summary and raw output below.");
				break;
			}
		}
		if (m_operationOutput != nullptr)
		{
			const bool compactOutput = state == RunState::Done;
			m_operationOutput->setMinimumHeight(compactOutput ? kOperationOutputMinHeight : kOperationOutputProminentMinHeight);
			m_operationOutput->setMaximumHeight(compactOutput ? kOperationOutputCompactMaxHeight : kOperationOutputMaxHeight);
			m_operationOutput->setPlainText(m_runOutputs.value(runId));
			m_operationOutput->moveCursor(QTextCursor::End);
		}
		if (m_copyOutputButton != nullptr)
		{
			const bool canCopyOutput = m_activityLogExpanded && m_operationOutput != nullptr && !m_operationOutput->toPlainText().isEmpty();
			m_copyOutputButton->setEnabled(canCopyOutput);
			m_copyOutputButton->setToolTip(canCopyOutput ? "Copy output for the selected run. Shortcut: Ctrl+Shift+C." : "Select a run to copy its output. Shortcut: Ctrl+Shift+C.");
		}
	}

	void LauncherMainWindow::SetActivityLogExpanded(bool expanded)
	{
		m_activityLogExpanded = expanded;
		if (m_activityPanel != nullptr)
		{
			m_activityPanel->setMinimumHeight(expanded ? kActivityPanelExpandedHeight : kActivityPanelCollapsedHeight);
			m_activityPanel->setMaximumHeight(expanded ? kActivityPanelExpandedHeight : kActivityPanelCollapsedHeight);
		}
		if (m_activityDetailsPanel != nullptr)
		{
			m_activityDetailsPanel->setVisible(expanded);
		}
		if (m_operationOutput != nullptr)
		{
			m_operationOutput->setVisible(expanded);
		}
		if (m_toggleOutputButton != nullptr)
		{
			m_toggleOutputButton->setText(QString::fromLatin1(expanded ? LauncherUi::Activity::CollapseGlyph : LauncherUi::Activity::ExpandGlyph));
			m_toggleOutputButton->setToolTip(expanded ? "Minimize recent runs and raw process output." : "Show recent runs and raw process output.");
			m_toggleOutputButton->setAccessibleDescription(m_toggleOutputButton->toolTip());
		}
		if (m_copyOutputButton != nullptr)
		{
			const bool canCopyOutput = expanded && m_operationOutput != nullptr && !m_operationOutput->toPlainText().isEmpty();
			m_copyOutputButton->setEnabled(canCopyOutput);
		}
	}

	void LauncherMainWindow::UpdateActivityRunSelectionVisuals()
	{
		for (auto it = m_runItemWidgets.begin(); it != m_runItemWidgets.end(); ++it)
		{
			const bool isSelected = it.key() == m_activeRunId;
			if (it.value().Root != nullptr)
			{
				it.value().Root->setProperty("Selected", isSelected);
				it.value().Root->style()->unpolish(it.value().Root);
				it.value().Root->style()->polish(it.value().Root);
			}
		}
	}

	void LauncherMainWindow::UpdateProgress()
	{
		if (m_activityDetailsPanel == nullptr)
		{
			return;
		}

		const bool hasRuns = m_startedRunCount > 0;
		if (m_activityDetailsPanel != nullptr)
		{
			m_activityDetailsPanel->setVisible(hasRuns && m_activityLogExpanded);
		}
		if (m_copyOutputButton != nullptr && !hasRuns)
		{
			m_copyOutputButton->setEnabled(false);
		}
	}

}
