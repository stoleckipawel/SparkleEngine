#include "LauncherActivityPanel.h"

#include "LauncherIconLibrary.h"
#include "LauncherUiDesign.h"
#include "Models/LauncherOperationResult.h"

#include <QtGui/QClipboard>
#include <QtGui/QColor>
#include <QtGui/QGuiApplication>
#include <QtGui/QKeySequence>
#include <QtGui/QTextCursor>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QListWidgetItem>
#include <QtWidgets/QProgressBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QStyle>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

#include <algorithm>

namespace SparkleLauncher
{
	static constexpr int kMaxOperationOutputCharacters = 1000000;

	static int ProgressPercentage(quint64 completed, quint64 total)
	{
		if (total == 0)
		{
			return 0;
		}
		const quint64 boundedCompleted = std::min(completed, total);
		return static_cast<int>(static_cast<long double>(boundedCompleted) * 100.0L / static_cast<long double>(total));
	}

	LauncherActivityPanel::LauncherActivityPanel(const LauncherIconLibrary& icons, const std::function<void(QWidget*)>& registerFocusable, QWidget* parent) :
	    QFrame(parent),
	    m_queuedIcon(icons.Icon(LauncherIcon::Queued, QColor(LauncherUi::Color::StateQueued))),
	    m_runningIcon(icons.Icon(LauncherIcon::Running, QColor(LauncherUi::Color::StateRunning))),
	    m_doneIcon(icons.Icon(LauncherIcon::Done, QColor(LauncherUi::Color::StateSuccess))),
	    m_canceledIcon(icons.Icon(LauncherIcon::Failed, QColor(LauncherUi::Color::StateWarning))),
	    m_failedIcon(icons.Icon(LauncherIcon::Failed, QColor(LauncherUi::Color::StateDestructive)))
	{
		setObjectName("ActivityBottomPanel");
		setMinimumHeight(LauncherUi::Activity::CollapsedHeight);
		setMaximumHeight(LauncherUi::Activity::CollapsedHeight);

		QVBoxLayout* layout = new QVBoxLayout(this);
		layout->setContentsMargins(0, 0, 0, 0);
		layout->setSpacing(0);

		QFrame* header = new QFrame(this);
		header->setObjectName("ActivityHeader");
		QHBoxLayout* headerLayout = new QHBoxLayout(header);
		headerLayout->setContentsMargins(LauncherUi::Activity::HeaderMargins);
		headerLayout->setSpacing(LauncherUi::Space::Small);

		QLabel* activityTitle = new QLabel("Activity", header);
		activityTitle->setObjectName("OutputPaneLabel");
		headerLayout->addWidget(activityTitle, 0);
		headerLayout->addStretch(1);

		m_toggleOutputButton = new QPushButton(QString::fromLatin1(LauncherUi::Activity::ExpandGlyph), header);
		m_toggleOutputButton->setObjectName("ActivityToggleButton");
		m_toggleOutputButton->setFixedSize(LauncherUi::Activity::ToggleButtonSize);
		m_toggleOutputButton->setEnabled(false);
		m_toggleOutputButton->setVisible(false);
		m_toggleOutputButton->setToolTip("Run a workflow to view its activity.");
		m_toggleOutputButton->setAccessibleName("Toggle Activity panel");
		m_toggleOutputButton->setAccessibleDescription("Shows or minimizes recent runs and raw process output.");
		registerFocusable(m_toggleOutputButton);
		connect(m_toggleOutputButton, &QPushButton::clicked, this, [this]() { ToggleExpanded(); });
		headerLayout->addWidget(m_toggleOutputButton, 0);
		layout->addWidget(header, 0);

		m_detailsPanel = new QFrame(this);
		m_detailsPanel->setObjectName("ActivityDetailsPanel");
		QHBoxLayout* activityLayout = new QHBoxLayout(m_detailsPanel);
		activityLayout->setContentsMargins(0, 0, 0, 0);
		activityLayout->setSpacing(0);

		QFrame* activityRail = new QFrame(m_detailsPanel);
		activityRail->setObjectName("ActivityRail");
		activityRail->setMinimumWidth(LauncherUi::Activity::ListWidth);
		QVBoxLayout* activityRailLayout = new QVBoxLayout(activityRail);
		activityRailLayout->setContentsMargins(LauncherUi::Activity::RailMargins);
		activityRailLayout->setSpacing(LauncherUi::Space::XSmall - 1);

		QLabel* activityHeader = new QLabel("Runs", activityRail);
		activityHeader->setObjectName("OutputPaneLabel");
		activityRailLayout->addWidget(activityHeader, 0);

		m_runList = new QListWidget(activityRail);
		m_runList->setObjectName("ActivityList");
		m_runList->setAccessibleName("Activity runs");
		m_runList->setAccessibleDescription("Recent runs. Select one to review its summary and output.");
		registerFocusable(m_runList);
		connect(m_runList, &QListWidget::currentItemChanged, this, [this](QListWidgetItem* current, QListWidgetItem*) { DisplaySelectedRunOutput(current); });
		activityRailLayout->addWidget(m_runList, 1);
		activityLayout->addWidget(activityRail, 0);

		QFrame* outputPane = new QFrame(m_detailsPanel);
		outputPane->setObjectName("OutputPane");
		QVBoxLayout* outputLayout = new QVBoxLayout(outputPane);
		outputLayout->setContentsMargins(LauncherUi::Activity::OutputMargins);
		outputLayout->setSpacing(LauncherUi::Space::XSmall - 1);

		QHBoxLayout* outputHeaderLayout = new QHBoxLayout();
		outputHeaderLayout->setContentsMargins(0, 0, 0, 0);
		outputHeaderLayout->setSpacing(LauncherUi::Space::Small - 2);
		QLabel* outputHeader = new QLabel("Log", outputPane);
		outputHeader->setObjectName("OutputPaneLabel");
		outputHeaderLayout->addWidget(outputHeader, 0);
		outputHeaderLayout->addStretch(1);

		m_copyOutputButton = new QPushButton("Copy output", outputPane);
		m_copyOutputButton->setObjectName("SecondaryButton");
		m_copyOutputButton->setIcon(icons.Icon(LauncherIcon::Copy, QColor(LauncherUi::Color::StateQueued)));
		m_copyOutputButton->setIconSize(QSize(LauncherUi::Icon::DefaultSize, LauncherUi::Icon::DefaultSize));
		m_copyOutputButton->setEnabled(false);
		m_copyOutputButton->setVisible(false);
		m_copyOutputButton->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_C));
		m_copyOutputButton->setToolTip("Select a run to copy its output. Shortcut: Ctrl+Shift+C.");
		m_copyOutputButton->setAccessibleName("Copy selected run output");
		m_copyOutputButton->setAccessibleDescription("Copies output for the selected run.");
		registerFocusable(m_copyOutputButton);
		connect(m_copyOutputButton, &QPushButton::clicked, this, [this]() { CopySelectedRunOutput(); });
		outputHeaderLayout->addWidget(m_copyOutputButton, 0);
		outputLayout->addLayout(outputHeaderLayout);

		m_selectedRunSummary = new QLabel("Select a run to view output.", outputPane);
		m_selectedRunSummary->setObjectName("ActivitySummary");
		m_selectedRunSummary->setAccessibleName("Selected activity summary");
		m_selectedRunSummary->setWordWrap(true);
		outputLayout->addWidget(m_selectedRunSummary);

		m_progressBar = new QProgressBar(outputPane);
		m_progressBar->setObjectName("ActivityProgress");
		m_progressBar->setAccessibleName("Selected activity progress");
		m_progressBar->setTextVisible(true);
		m_progressBar->setVisible(false);
		outputLayout->addWidget(m_progressBar);

		m_operationOutput = new QTextEdit(outputPane);
		m_operationOutput->setObjectName("OperationOutput");
		m_operationOutput->setReadOnly(true);
		m_operationOutput->setToolTip("Select a run to view its output.");
		m_operationOutput->setAccessibleName("Selected run output");
		m_operationOutput->setAccessibleDescription("Read-only output for the selected run.");
		m_operationOutput->setMinimumHeight(LauncherUi::OperationOutput::MinHeight);
		m_operationOutput->setMaximumHeight(LauncherUi::OperationOutput::MaxHeight);
		registerFocusable(m_operationOutput);
		outputLayout->addWidget(m_operationOutput);
		activityLayout->addWidget(outputPane, 1);
		layout->addWidget(m_detailsPanel, 1);

		SetExpanded(false);
	}

	void LauncherActivityPanel::ShowMessage(const QString& message)
	{
		m_operationOutput->setPlainText(message);
	}

	void LauncherActivityPanel::RegisterRun(const QString& runId, const QString& title)
	{
		QListWidgetItem* item = new QListWidgetItem(m_runList);
		item->setData(Qt::UserRole, runId);
		item->setSizeHint(QSize(0, LauncherUi::Activity::HistoryRowHeight));
		item->setText(QString());
		const RunWidgets rowWidgets = CreateRunWidgets(title);
		m_runList->setItemWidget(item, rowWidgets.Root);
		m_runs.insert(runId, {item, rowWidgets, RunState::Queued, title, title + " queued.\n"});
		SetRunState(runId, RunState::Queued, title);
		m_runList->setCurrentItem(item);
		m_activeRunId = runId;
		SetExpanded(m_expanded);
	}

	void LauncherActivityPanel::DisplayOperationStarted(const QString& runId, const QString& title)
	{
		const auto run = m_runs.constFind(runId);
		const QString effectiveTitle = run == m_runs.constEnd() ? title : run->Title;
		SetRunState(runId, RunState::Running, effectiveTitle);
		AppendRunOutput(runId, effectiveTitle + " started.\n");
		ShowRunOutput(runId);
		SetExpanded(true);
	}

	void LauncherActivityPanel::AppendOperationOutput(const QString& runId, const QString& outputText)
	{
		AppendRunOutput(runId, outputText);
		if (m_activeRunId == runId)
		{
			ShowRunOutput(runId);
		}
	}

	void LauncherActivityPanel::UpdateOperationProgress(const QString& runId, const QString& phase, quint64 completed, quint64 total)
	{
		auto run = m_runs.find(runId);
		if (run == m_runs.end() || run->State != RunState::Running)
		{
			return;
		}

		run->HasProgress = true;
		run->ProgressPhase = phase;
		run->ProgressCompleted = completed;
		run->ProgressTotal = total;
		if (m_activeRunId == runId)
		{
			ShowRunOutput(runId);
		}
		else
		{
			UpdateRunProgressPresentation(*run);
		}
	}

	QString LauncherActivityPanel::DisplayOperationFinished(const QString& runId, const QString& title, const LauncherOperationResult& result)
	{
		auto run = m_runs.find(runId);
		const QString effectiveTitle = run == m_runs.end() ? title : run->Title;
		const RunState terminalState = result.Succeeded ? RunState::Done : result.Skipped ? RunState::Blocked : result.Canceled ? RunState::Canceled : RunState::Failed;
		SetRunState(runId, terminalState, effectiveTitle);

		if (result.Succeeded)
		{
			AppendRunOutput(runId, "\nResult: " + result.Status + "\n");
		}
		else
		{
			run = m_runs.find(runId);
			if (run == m_runs.end())
			{
				return effectiveTitle;
			}
			QString diagnosis = "Result: " + result.Status + "\n";
			diagnosis += "Problem type: " + result.ProblemKind + "\n";
			diagnosis += "What failed: " + result.Problem + "\n";
			diagnosis += "Next action: " + result.ExpectedAction + "\n";
			if (result.HasExitCode)
			{
				diagnosis += QStringLiteral("Exit code: %1\n").arg(result.ExitCode);
			}
			if (!result.LogPath.isEmpty())
			{
				diagnosis += "Log: " + result.LogPath + "\n";
			}
			run->Output = diagnosis + "\nProcess output:\n" + run->Output;
		}

		ShowRunOutput(runId);
		SetExpanded(true);
		return effectiveTitle;
	}

	void LauncherActivityPanel::DisplayBlockedOperation(const QString& runId, const QString& title, const QString& message)
	{
		RegisterRun(runId, title);
		SetRunState(runId, RunState::Blocked, title);

		AppendRunOutput(
		    runId,
		    QStringLiteral(
		        "Result: Blocked\nProblem type: Prerequisite\nWhat failed: %1\nNext action: Resolve the reported prerequisite, then retry "
		        "Quick Start.\n")
		        .arg(message));

		ShowRunOutput(runId);
		SetExpanded(true);
	}

	void LauncherActivityPanel::AppendRunOutput(const QString& runId, const QString& text)
	{
		auto run = m_runs.find(runId);
		if (run == m_runs.end())
		{
			return;
		}

		run->Output += text;
		const int overflowCharacters = run->Output.size() - kMaxOperationOutputCharacters;
		if (overflowCharacters > 0)
		{
			run->Output.remove(0, overflowCharacters);
		}
	}

	void LauncherActivityPanel::UpdateRunProgressPresentation(RunRecord& run)
	{
		if (run.State != RunState::Running || run.Widgets.StateLabel == nullptr)
		{
			return;
		}

		QString stateText = "Running";
		if (run.ProgressTotal != 0)
		{
			stateText += QStringLiteral(" · %1%").arg(ProgressPercentage(run.ProgressCompleted, run.ProgressTotal));
		}
		run.Widgets.StateLabel->setText(stateText);
		if (run.Item != nullptr)
		{
			run.Item->setData(Qt::AccessibleTextRole, stateText + ": " + run.Title);
		}
	}

	void LauncherActivityPanel::ShowRunOutput(const QString& runId)
	{
		const auto run = m_runs.constFind(runId);
		if (run == m_runs.constEnd())
		{
			return;
		}

		m_activeRunId = runId;
		UpdateRunSelectionVisuals();
		const RunState state = run->State;
		const QString& title = run->Title;
		QString stateName;
		switch (state)
		{
			case RunState::Queued:
				stateName = "queued";

				m_selectedRunSummary->setText("Queued: " + title + ". Waiting to start.");
				break;

			case RunState::Running:
				stateName = "running";

				if (run->HasProgress)
				{
					QString progressText = run->ProgressPhase;
					if (run->ProgressTotal != 0)
					{
						progressText += QStringLiteral(" — %1/%2 (%3%)")
						                    .arg(static_cast<qulonglong>(run->ProgressCompleted))
						                    .arg(static_cast<qulonglong>(run->ProgressTotal))
						                    .arg(ProgressPercentage(run->ProgressCompleted, run->ProgressTotal));
					}
					m_selectedRunSummary->setText("Running: " + title + ". " + progressText + ".");
				}
				else
				{
					m_selectedRunSummary->setText("Running: " + title + ". Waiting for progress from the active tool.");
				}
				break;

			case RunState::Done:
				stateName = "done";

				m_selectedRunSummary->setText("Done: " + title + ". Output is available below.");
				break;

			case RunState::Blocked:
				stateName = "blocked";

				m_selectedRunSummary->setText("Blocked: " + title + ". Follow the next action below.");
				break;

			case RunState::Canceled:
				stateName = "canceled";

				m_selectedRunSummary->setText("Canceled: " + title + ". Run it again when ready.");
				break;

			case RunState::Failed:
				stateName = "failed";

				m_selectedRunSummary->setText("Failed: " + title + ". Follow the next action below.");
				break;
		}
		m_selectedRunSummary->setProperty("RunState", stateName);
		m_selectedRunSummary->style()->unpolish(m_selectedRunSummary);
		m_selectedRunSummary->style()->polish(m_selectedRunSummary);

		const bool showProgress = state == RunState::Running && run->HasProgress;
		m_progressBar->setVisible(showProgress);
		if (showProgress)
		{
			if (run->ProgressTotal == 0)
			{
				m_progressBar->setRange(0, 0);
				m_progressBar->setFormat(run->ProgressPhase);
			}
			else
			{
				m_progressBar->setRange(0, 100);
				m_progressBar->setValue(ProgressPercentage(run->ProgressCompleted, run->ProgressTotal));
				m_progressBar->setFormat(run->ProgressPhase + QStringLiteral(" — %p%"));
			}
			m_progressBar->setAccessibleDescription(m_selectedRunSummary->text());
		}

		const bool compactOutput = state == RunState::Done;
		m_operationOutput->setMinimumHeight(compactOutput ? LauncherUi::OperationOutput::MinHeight : LauncherUi::OperationOutput::ProminentMinHeight);
		m_operationOutput->setMaximumHeight(compactOutput ? LauncherUi::OperationOutput::CompactMaxHeight : LauncherUi::OperationOutput::MaxHeight);
		m_operationOutput->setPlainText(run->Output);
		m_operationOutput->moveCursor(QTextCursor::End);

		const bool canCopyOutput = m_expanded && !m_operationOutput->toPlainText().isEmpty();
		m_copyOutputButton->setVisible(!runId.isEmpty());
		m_copyOutputButton->setEnabled(canCopyOutput);
		m_copyOutputButton->setToolTip(canCopyOutput ? "Copy output for the selected run. Shortcut: Ctrl+Shift+C." : "Select a run to copy its output. Shortcut: Ctrl+Shift+C.");
	}

	LauncherActivityPanel::RunWidgets LauncherActivityPanel::CreateRunWidgets(const QString& title)
	{
		RunWidgets widgets;
		QWidget* row = new QWidget(m_runList);
		row->setObjectName("ActivityRunRow");
		row->setFixedHeight(LauncherUi::Activity::RowHeight);
		QHBoxLayout* rowLayout = new QHBoxLayout(row);
		rowLayout->setContentsMargins(0, 0, 0, 0);
		rowLayout->setSpacing(LauncherUi::Space::Small - 2);

		QFrame* indicator = new QFrame(row);
		indicator->setObjectName("ActivityRunIndicator");
		indicator->setFixedWidth(LauncherUi::Activity::RunIndicatorWidth);
		rowLayout->addWidget(indicator, 0);

		QVBoxLayout* textLayout = new QVBoxLayout();
		textLayout->setContentsMargins(0, 0, 0, 0);
		textLayout->setSpacing(0);
		QLabel* titleLabel = new QLabel(title, row);
		titleLabel->setObjectName("ActivityRunTitle");
		titleLabel->setWordWrap(false);
		QLabel* stateLabel = new QLabel("Queued", row);
		stateLabel->setObjectName("ActivityRunState");
		textLayout->addWidget(titleLabel);
		textLayout->addWidget(stateLabel);
		rowLayout->addLayout(textLayout, 1);

		widgets.Root = row;
		widgets.Indicator = indicator;
		widgets.TitleLabel = titleLabel;
		widgets.StateLabel = stateLabel;
		return widgets;
	}

	QIcon LauncherActivityPanel::IconForState(RunState state) const
	{
		switch (state)
		{
			case RunState::Queued:
				return m_queuedIcon;
			case RunState::Running:
				return m_runningIcon;
			case RunState::Done:
				return m_doneIcon;
			case RunState::Blocked:
				return m_canceledIcon;
			case RunState::Canceled:
				return m_canceledIcon;
			case RunState::Failed:
				return m_failedIcon;
		}

		return {};
	}

	void LauncherActivityPanel::DisplaySelectedRunOutput(QListWidgetItem* currentItem)
	{
		if (currentItem == nullptr)
		{
			return;
		}

		const QString runId = currentItem->data(Qt::UserRole).toString();
		ShowRunOutput(runId);
		const auto run = m_runs.constFind(runId);
		const RunState state = run == m_runs.constEnd() ? RunState::Done : run->State;
		if (state == RunState::Running || state == RunState::Blocked || state == RunState::Canceled || state == RunState::Failed)
		{
			SetExpanded(true);
		}
	}

	void LauncherActivityPanel::CopySelectedRunOutput()
	{
		QGuiApplication::clipboard()->setText(m_operationOutput->toPlainText());
	}

	void LauncherActivityPanel::ToggleExpanded()
	{
		if (m_runs.isEmpty())
		{
			return;
		}
		SetExpanded(!m_expanded);
	}

	void LauncherActivityPanel::SetRunState(const QString& runId, RunState state, const QString& title)
	{
		auto run = m_runs.find(runId);
		if (run == m_runs.end() || run->Item == nullptr)
		{
			return;
		}

		run->State = state;
		run->Title = title;

		QString stateText;
		switch (state)
		{
			case RunState::Queued:
				stateText = "Queued";

				break;

			case RunState::Running:
				stateText = "Running";

				break;

			case RunState::Done:
				stateText = "Done";

				break;

			case RunState::Blocked:
				stateText = "Blocked";

				break;

			case RunState::Canceled:
				stateText = "Canceled";

				break;

			case RunState::Failed:
				stateText = "Failed";

				break;
		}

		run->Item->setText(QString());
		run->Item->setIcon(IconForState(state));
		run->Item->setData(Qt::UserRole + 1, stateText);
		run->Item->setData(Qt::AccessibleTextRole, stateText + ": " + title);
		run->Item->setData(Qt::AccessibleDescriptionRole, "Launcher activity run " + stateText.toLower());
		run->Item->setToolTip(stateText + ": " + title);

		const RunWidgets& widgets = run->Widgets;
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
		UpdateRunSelectionVisuals();
	}

	void LauncherActivityPanel::SetExpanded(bool expanded)
	{
		const bool hasRuns = !m_runs.isEmpty();
		expanded = expanded && hasRuns;
		m_expanded = expanded;
		setMinimumHeight(expanded ? LauncherUi::Activity::ExpandedHeight : LauncherUi::Activity::CollapsedHeight);
		setMaximumHeight(expanded ? LauncherUi::Activity::ExpandedHeight : LauncherUi::Activity::CollapsedHeight);
		m_detailsPanel->setVisible(expanded);
		m_operationOutput->setVisible(expanded);

		m_toggleOutputButton->setEnabled(hasRuns);
		m_toggleOutputButton->setVisible(hasRuns);
		m_toggleOutputButton->setText(QString::fromLatin1(expanded ? LauncherUi::Activity::CollapseGlyph : LauncherUi::Activity::ExpandGlyph));
		m_toggleOutputButton->setToolTip(!hasRuns ? "Run a workflow to view its activity." : expanded ? "Minimize recent runs and raw process output." : "Show recent runs and raw process output.");
		m_toggleOutputButton->setAccessibleDescription(m_toggleOutputButton->toolTip());

		const bool canCopyOutput = expanded && !m_operationOutput->toPlainText().isEmpty();
		m_copyOutputButton->setVisible(expanded && !m_activeRunId.isEmpty());
		m_copyOutputButton->setEnabled(canCopyOutput);
	}

	void LauncherActivityPanel::UpdateRunSelectionVisuals()
	{
		for (auto it = m_runs.begin(); it != m_runs.end(); ++it)
		{
			const bool selected = it.key() == m_activeRunId;
			if (it->Widgets.Root != nullptr)
			{
				it->Widgets.Root->setProperty("Selected", selected);
				it->Widgets.Root->style()->unpolish(it->Widgets.Root);
				it->Widgets.Root->style()->polish(it->Widgets.Root);
			}
		}
	}
}
