#include "LauncherOutputWidgets.h"

#include <QtGui/QKeySequence>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QVBoxLayout>

namespace SparkleLauncher
{
	namespace
	{
		constexpr int kActivityListWidth = 280;
		constexpr int kLauncherIconSize = 14;
	}

	LauncherOutputPanelWidgets CreateLauncherOutputPanel(
	    QWidget* parent,
	    const QIcon& copyIcon,
	    const QSize& copyIconSize,
	    std::function<void(QWidget*)> registerFocusable,
	    std::function<void()> onCopyOutput,
	    std::function<void(QListWidgetItem*, QListWidgetItem*)> onCurrentRunChanged)
	{
		LauncherOutputPanelWidgets widgets;

		QFrame* panel = new QFrame(parent);
		panel->setObjectName("OutputPanel");
		QVBoxLayout* layout = new QVBoxLayout(panel);
		layout->setContentsMargins(0, 0, 0, 0);
		layout->setSpacing(0);

		QHBoxLayout* activityLayout = new QHBoxLayout();
		activityLayout->setContentsMargins(0, 0, 0, 0);
		activityLayout->setSpacing(0);

		QFrame* activityRail = new QFrame(panel);
		activityRail->setObjectName("ActivityRail");
		activityRail->setMinimumWidth(kActivityListWidth);
		QVBoxLayout* activityRailLayout = new QVBoxLayout(activityRail);
		activityRailLayout->setContentsMargins(4, 4, 4, 4);
		activityRailLayout->setSpacing(3);

		QLabel* activityHeader = new QLabel("Runs", activityRail);
		activityHeader->setObjectName("OutputPaneLabel");
		activityRailLayout->addWidget(activityHeader, 0);

		QListWidget* activityList = new QListWidget(panel);
		activityList->setObjectName("ActivityList");
		activityList->setAccessibleName("Activity runs");
		activityList->setAccessibleDescription("Recent runs. Select one to review its summary and output.");
		registerFocusable(activityList);
		QObject::connect(activityList, &QListWidget::currentItemChanged, panel, [onCurrentRunChanged](QListWidgetItem* current, QListWidgetItem* previous) {
			onCurrentRunChanged(current, previous);
		});
		activityRailLayout->addWidget(activityList, 1);
		activityLayout->addWidget(activityRail, 0);

		QFrame* outputPane = new QFrame(panel);
		outputPane->setObjectName("OutputPane");
		QVBoxLayout* outputLayout = new QVBoxLayout(outputPane);
		outputLayout->setContentsMargins(6, 4, 6, 6);
		outputLayout->setSpacing(3);

		QHBoxLayout* outputHeaderLayout = new QHBoxLayout();
		outputHeaderLayout->setContentsMargins(0, 0, 0, 0);
		outputHeaderLayout->setSpacing(6);
		QLabel* outputHeader = new QLabel("Log", outputPane);
		outputHeader->setObjectName("OutputPaneLabel");
		outputHeaderLayout->addWidget(outputHeader, 0);
		outputHeaderLayout->addStretch(1);

		QPushButton* copyOutputButton = new QPushButton("Copy output", panel);
		copyOutputButton->setObjectName("SecondaryButton");
		copyOutputButton->setIcon(copyIcon);
		copyOutputButton->setIconSize(copyIconSize.isValid() ? copyIconSize : QSize(kLauncherIconSize, kLauncherIconSize));
		copyOutputButton->setEnabled(false);
		copyOutputButton->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_C));
		copyOutputButton->setToolTip("Select a run to copy its output. Shortcut: Ctrl+Shift+C.");
		copyOutputButton->setAccessibleName("Copy selected run output");
		copyOutputButton->setAccessibleDescription("Copies output for the selected run.");
		registerFocusable(copyOutputButton);
		QObject::connect(copyOutputButton, &QPushButton::clicked, panel, [onCopyOutput]() {
			onCopyOutput();
		});
		outputHeaderLayout->addWidget(copyOutputButton, 0);
		outputLayout->addLayout(outputHeaderLayout);

		QLabel* selectedRunSummary = new QLabel("Select a run to view output.", panel);
		selectedRunSummary->setObjectName("ActivitySummary");
		selectedRunSummary->setAccessibleName("Selected activity summary");
		selectedRunSummary->setWordWrap(true);
		outputLayout->addWidget(selectedRunSummary);

		QTextEdit* operationOutput = new QTextEdit(panel);
		operationOutput->setObjectName("OperationOutput");
		operationOutput->setReadOnly(true);
		operationOutput->setToolTip("Select a run to view its output.");
		operationOutput->setAccessibleName("Selected run output");
		operationOutput->setAccessibleDescription("Read-only output for the selected run.");
		registerFocusable(operationOutput);
		outputLayout->addWidget(operationOutput);
		activityLayout->addWidget(outputPane, 1);

		layout->addLayout(activityLayout, 1);

		widgets.Root = panel;
		widgets.ActivityDetailsPanel = panel;
		widgets.ActivityList = activityList;
		widgets.SelectedRunSummary = selectedRunSummary;
		widgets.OperationOutput = operationOutput;
		widgets.CopyOutputButton = copyOutputButton;
		return widgets;
	}

	LauncherActivityRowWidgets CreateLauncherActivityRow(QWidget* parent, const QString& title)
	{
		LauncherActivityRowWidgets widgets;

		QWidget* row = new QWidget(parent);
		row->setObjectName("ActivityRunRow");
		row->setFixedHeight(26);
		QHBoxLayout* rowLayout = new QHBoxLayout(row);
		rowLayout->setContentsMargins(0, 0, 0, 0);
		rowLayout->setSpacing(6);

		QFrame* indicator = new QFrame(row);
		indicator->setObjectName("ActivityRunIndicator");
		indicator->setFixedWidth(4);
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
}
