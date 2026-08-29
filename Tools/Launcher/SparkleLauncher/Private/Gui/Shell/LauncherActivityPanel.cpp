#include "LauncherActivityPanel.h"

#include "LauncherIconLibrary.h"
#include "LauncherUiDesign.h"

#include <QtGui/QClipboard>
#include <QtGui/QColor>
#include <QtGui/QGuiApplication>
#include <QtGui/QKeySequence>
#include <QtGui/QTextCursor>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QListWidgetItem>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QStyle>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

#include <utility>

namespace SparkleLauncher
{
	static constexpr int kMaxOperationOutputCharacters = 1000000;

	LauncherActivityPanel::LauncherActivityPanel(
	    const LauncherIconLibrary& icons,
	    std::function<void(QWidget*)> registerFocusable,
	    QWidget* parent) :
	    QFrame(parent),
	    m_queuedIcon(icons.Icon(LauncherIcon::Queued, QColor(LauncherUi::Color::StateQueued))),
	    m_runningIcon(icons.Icon(LauncherIcon::Running, QColor(LauncherUi::Color::StateRunning))),
	    m_doneIcon(icons.Icon(LauncherIcon::Done, QColor(LauncherUi::Color::StateSuccess))),
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
		connect(
		    m_runList,
		    &QListWidget::currentItemChanged,
		    this,
		    [this](QListWidgetItem* current, QListWidgetItem*) { DisplaySelectedRunOutput(current); });
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
		m_runItems.insert(runId, item);
		m_runWidgets.insert(runId, rowWidgets);
		m_runTitles.insert(runId, title);
		SetRunState(runId, RunState::Queued, title);
		m_runOutputs.insert(runId, title + " queued.\n");
		m_runList->setCurrentItem(item);
		m_activeRunId = runId;
		SetExpanded(m_expanded);
	}

	void LauncherActivityPanel::DisplayOperationStarted(const QString& runId, const QString& title)
	{
		const QString effectiveTitle = m_runTitles.value(runId, title);
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

	QString LauncherActivityPanel::DisplayOperationFinished(
	    const QString& runId,
	    const QString& title,
	    const QString& statusText,
	    int exitCode,
	    const QString& recoveryHint)
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
			const QString recoveryText = recoveryHint.isEmpty() ? QString() : QStringLiteral("Recovery: %1\n\n").arg(recoveryHint);
			m_runOutputs.insert(
			    runId,
			    QStringLiteral("Failed: %1 (exit code %2)\n").arg(statusText).arg(exitCode) + recoveryText + "\n" + existingOutput + "\n"
			        + effectiveTitle + " finished: " + statusText + "\n");
		}

		ShowRunOutput(runId);
		SetExpanded(true);
		return effectiveTitle;
	}

	void LauncherActivityPanel::DisplayBlockedOperation(const QString& runId, const QString& title, const QString& message)
	{
		RegisterRun(runId, title);
		SetRunState(runId, RunState::Failed, title);
		AppendRunOutput(runId, QStringLiteral("Quick Start is blocked.\n\n%1\n").arg(message));
		ShowRunOutput(runId);
		SetExpanded(true);
	}

	void LauncherActivityPanel::AppendRunOutput(const QString& runId, const QString& text)
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

	void LauncherActivityPanel::ShowRunOutput(const QString& runId)
	{
		m_activeRunId = runId;
		UpdateRunSelectionVisuals();
		const RunState state = m_runStates.value(runId, RunState::Queued);
		const QString title = m_runTitles.value(runId, "Selected run");
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
			case RunState::Canceled:
				m_selectedRunSummary->setText("Canceled: " + title + ". Cleanup is continuing as a separate run.");
				break;
			case RunState::Failed:
				m_selectedRunSummary->setText("Failed: " + title + ". Review the summary and raw output below.");
				break;
		}

		const bool compactOutput = state == RunState::Done || state == RunState::Canceled;
		m_operationOutput->setMinimumHeight(
		    compactOutput ? LauncherUi::OperationOutput::MinHeight : LauncherUi::OperationOutput::ProminentMinHeight);
		m_operationOutput->setMaximumHeight(
		    compactOutput ? LauncherUi::OperationOutput::CompactMaxHeight : LauncherUi::OperationOutput::MaxHeight);
		m_operationOutput->setPlainText(m_runOutputs.value(runId));
		m_operationOutput->moveCursor(QTextCursor::End);

		const bool canCopyOutput = m_expanded && !m_operationOutput->toPlainText().isEmpty();
		m_copyOutputButton->setVisible(!runId.isEmpty());
		m_copyOutputButton->setEnabled(canCopyOutput);
		m_copyOutputButton->setToolTip(
		    canCopyOutput ? "Copy output for the selected run. Shortcut: Ctrl+Shift+C."
		                  : "Select a run to copy its output. Shortcut: Ctrl+Shift+C.");
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
			case RunState::Canceled:
				return m_queuedIcon;
			case RunState::Running:
				return m_runningIcon;
			case RunState::Done:
				return m_doneIcon;
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
		const RunState state = m_runStates.value(runId, RunState::Done);
		if (state == RunState::Running || state == RunState::Failed)
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
		if (m_runItems.isEmpty())
		{
			return;
		}
		SetExpanded(!m_expanded);
	}

	void LauncherActivityPanel::SetRunState(const QString& runId, RunState state, const QString& title)
	{
		QListWidgetItem* item = m_runItems.value(runId, nullptr);
		if (item == nullptr)
		{
			return;
		}

		m_runStates.insert(runId, state);
		m_runTitles.insert(runId, title);

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
			case RunState::Canceled:
				stateText = "Canceled";
				break;
			case RunState::Failed:
				stateText = "Failed";
				break;
		}

		item->setText(QString());
		item->setIcon(IconForState(state));
		item->setData(Qt::UserRole + 1, stateText);
		item->setData(Qt::AccessibleTextRole, stateText + ": " + title);
		item->setData(Qt::AccessibleDescriptionRole, "Launcher activity run " + stateText.toLower());
		item->setToolTip(stateText + ": " + title);

		const RunWidgets widgets = m_runWidgets.value(runId);
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
		const bool hasRuns = !m_runItems.isEmpty();
		expanded = expanded && hasRuns;
		m_expanded = expanded;
		setMinimumHeight(expanded ? LauncherUi::Activity::ExpandedHeight : LauncherUi::Activity::CollapsedHeight);
		setMaximumHeight(expanded ? LauncherUi::Activity::ExpandedHeight : LauncherUi::Activity::CollapsedHeight);
		m_detailsPanel->setVisible(expanded);
		m_operationOutput->setVisible(expanded);

		m_toggleOutputButton->setEnabled(hasRuns);
		m_toggleOutputButton->setVisible(hasRuns);
		m_toggleOutputButton->setText(
		    QString::fromLatin1(expanded ? LauncherUi::Activity::CollapseGlyph : LauncherUi::Activity::ExpandGlyph));
		m_toggleOutputButton->setToolTip(
		    !hasRuns       ? "Run a workflow to view its activity."
		        : expanded ? "Minimize recent runs and raw process output."
		                   : "Show recent runs and raw process output.");
		m_toggleOutputButton->setAccessibleDescription(m_toggleOutputButton->toolTip());

		const bool canCopyOutput = expanded && !m_operationOutput->toPlainText().isEmpty();
		m_copyOutputButton->setVisible(expanded && !m_activeRunId.isEmpty());
		m_copyOutputButton->setEnabled(canCopyOutput);
	}

	void LauncherActivityPanel::UpdateRunSelectionVisuals()
	{
		for (auto it = m_runWidgets.begin(); it != m_runWidgets.end(); ++it)
		{
			const bool selected = it.key() == m_activeRunId;
			if (it.value().Root != nullptr)
			{
				it.value().Root->setProperty("Selected", selected);
				it.value().Root->style()->unpolish(it.value().Root);
				it.value().Root->style()->polish(it.value().Root);
			}
		}
	}
}
