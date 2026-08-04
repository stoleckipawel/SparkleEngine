#include "LauncherMainWindow.h"

#include "LauncherLayoutWidgets.h"
#include "LauncherOutputWidgets.h"
#include "LauncherContentModel.h"
#include "LauncherContextUiModel.h"
#include "LauncherOperationRequestFactory.h"
#include "LauncherSelectionWidgets.h"
#include "LauncherSettings.h"
#include "LauncherUiDesign.h"
#include "LauncherWorkflowCatalog.h"

#include "SparkleLauncher/BuildWorkspaceOperations.h"

#include <QtGui/QColor>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QFrame>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QScrollArea>
#include <QtWidgets/QSizePolicy>
#include <QtWidgets/QStackedWidget>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QVBoxLayout>

namespace SparkleLauncher
{
	static constexpr int kSpaceTiny = LauncherUi::Space::Tiny;
	static constexpr int kWorkflowRailWidth = LauncherUi::Shell::RailWidth;
	static constexpr int kWorkflowGroupMinHeight = LauncherUi::Shell::RailItemMinHeight;
	static constexpr int kWorkflowButtonMinHeight = LauncherUi::Shell::TabMinHeight;
	static constexpr int kOperationOutputMinHeight = LauncherUi::OperationOutput::MinHeight;
	static constexpr int kOperationOutputMaxHeight = LauncherUi::OperationOutput::MaxHeight;
	static constexpr int kActivityPanelCollapsedHeight = LauncherUi::Activity::CollapsedHeight;
	static constexpr int kLauncherIconSize = LauncherUi::Icon::DefaultSize;
	static constexpr const char* kColorStateQueued = LauncherUi::Color::StateQueued;

	static void ApplyContextComboMetrics(QComboBox& combo, int minWidth, int maxWidth)
	{
		combo.setMinimumWidth(minWidth);
		combo.setMaximumWidth(maxWidth);
		combo.setMinimumHeight(LauncherUi::HeaderContext::ComboHeight);
		combo.setMaximumHeight(LauncherUi::HeaderContext::ComboHeight);
	}

	static void PopulateBoundContextCombo(
	    QComboBox* combo,
	    const QVector<LauncherSelectionOption>& options,
	    const QString& currentValue,
	    LauncherSettings& settings,
	    void (LauncherSettings::*setter)(const QString&))
	{
		if (combo == nullptr)
		{
			return;
		}

		const QString effectiveValue = PopulateLauncherSelectionCombo(*combo, options, currentValue);
		if (!effectiveValue.isEmpty() && effectiveValue != currentValue)
		{
			(settings.*setter)(effectiveValue);
		}
	}

	QWidget* LauncherMainWindow::CreateWorkflowSurface()
	{
		QFrame* surface = new QFrame(this);
		surface->setObjectName("WorkflowSurface");
		QHBoxLayout* layout = new QHBoxLayout(surface);
		layout->setContentsMargins(0, 0, 0, 0);
		layout->setSpacing(0);
		layout->addWidget(CreateProcessPicker(surface), 0);
		layout->addWidget(CreateOptionsPanel(surface), 1);
		return surface;
	}

	QWidget* LauncherMainWindow::CreateProcessPicker(QWidget* parent)
	{
		QFrame* panel = new QFrame(parent);
		panel->setObjectName("ProcessPanel");
		panel->setFixedWidth(kWorkflowRailWidth);
		QVBoxLayout* layout = new QVBoxLayout(panel);
		layout->setContentsMargins(0, 0, 0, LauncherUi::Shell::RailBottomPadding);
		layout->setSpacing(LauncherUi::Space::XSmall);

		QVBoxLayout* groupLayout = new QVBoxLayout();
		groupLayout->setContentsMargins(0, 0, 0, 0);
		groupLayout->setSpacing(LauncherUi::Shell::RailGroupSpacing);

		m_workflowGroupButtonGroup = new QButtonGroup(this);
		m_workflowGroupButtonGroup->setExclusive(true);
		m_processButtonGroup = new QButtonGroup(this);
		m_processButtonGroup->setExclusive(true);

		m_operationStack = new QStackedWidget(panel);
		m_operationStack->setObjectName("OperationStack");

		const QVector<LauncherWorkflowDefinition> workflows = CreateLauncherWorkflowCatalog();
		for (int workflowIndex = 0; workflowIndex < workflows.size(); ++workflowIndex)
		{
			const LauncherWorkflowDefinition& workflow = workflows[workflowIndex];
			QToolButton* groupButton = new QToolButton(panel);
			groupButton->setText(workflow.Title);
			groupButton->setObjectName("WorkflowGroupButton");
			groupButton->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
			groupButton->setMinimumHeight(kWorkflowGroupMinHeight);
			groupButton->setMaximumHeight(kWorkflowGroupMinHeight);
			groupButton->setMinimumWidth(kWorkflowRailWidth);
			groupButton->setMaximumWidth(kWorkflowRailWidth);
			groupButton->setProperty("WorkflowIndex", workflowIndex);
			groupButton->setProperty("ActiveState", "false");
			groupButton->setAccessibleName(workflow.Title + " workflow group");
			groupButton->setIcon(WorkflowIconForKey(workflow.IconKey));
			groupButton->setIconSize(QSize(LauncherUi::Shell::RailIconSize, LauncherUi::Shell::RailIconSize));
			RegisterFocusable(groupButton);
			m_workflowGroupButtonGroup->addButton(groupButton);
			groupLayout->addWidget(groupButton);

			QWidget* tabPage = new QWidget(m_operationStack);
			QHBoxLayout* actionLayout = new QHBoxLayout();
			actionLayout->setContentsMargins(0, 0, 0, 0);
			actionLayout->setSpacing(LauncherUi::Shell::WorkflowTabSpacing);
			if (workflow.OperationIds.size() > 1)
			{
				for (int index = 0; index < workflow.OperationIds.size(); ++index)
				{
					const QString& operationId = workflow.OperationIds[index];
					QPushButton* button = CreateProcessButton(DisplayNameForOperation(operationId), operationId, tabPage);
					m_processButtonGroup->addButton(button);
					actionLayout->addWidget(button);
				}
			}
			actionLayout->addStretch(1);
			tabPage->setLayout(actionLayout);
			const int pageIndex = m_operationStack->addWidget(tabPage);
			for (const QString& operationId : workflow.OperationIds)
			{
				m_workflowPageByOperation.insert(operationId, pageIndex);
			}
		}
		groupLayout->addStretch(1);
		connect(m_workflowGroupButtonGroup, &QButtonGroup::buttonClicked, this, &LauncherMainWindow::SelectWorkflowGroupButton);
		connect(m_processButtonGroup, &QButtonGroup::buttonClicked, this, &LauncherMainWindow::SelectProcessButton);
		layout->addLayout(groupLayout, 1);
		return panel;
	}

	QPushButton* LauncherMainWindow::CreateProcessButton(const QString& label, const QString& operationId, QWidget* parent)
	{
		QPushButton* button = new QPushButton(label, parent);
		button->setObjectName("WorkflowButton");
		button->setCheckable(true);
		button->setMinimumHeight(kWorkflowButtonMinHeight);
		button->setProperty("OperationId", operationId);
		button->setAccessibleName(label + " workflow");
		RegisterFocusable(button);
		return button;
	}

	QWidget* LauncherMainWindow::CreateOptionsPanel(QWidget* parent)
	{
		QFrame* panel = new QFrame(parent);
		panel->setObjectName("OptionsPanel");
		QVBoxLayout* layout = new QVBoxLayout(panel);
		layout->setContentsMargins(0, 0, 0, 0);
		layout->setSpacing(0);

		QFrame* titleBand = new QFrame(panel);
		titleBand->setObjectName("TitleBand");
		QHBoxLayout* titleBandLayout = new QHBoxLayout(titleBand);
		titleBandLayout->setContentsMargins(LauncherUi::TitleBand::Margins);
		titleBandLayout->setSpacing(LauncherUi::TitleBand::Spacing);

		QVBoxLayout* titleStack = new QVBoxLayout();
		titleStack->setContentsMargins(0, 0, 0, 0);
		titleStack->setSpacing(0);
		m_activeOperationLabel = new QLabel("No workflow selected", titleBand);
		m_activeOperationLabel->setObjectName("ActiveOperationLabel");
		m_activeOperationLabel->setAccessibleName("Selected workflow");
		titleStack->addWidget(m_activeOperationLabel, 0, Qt::AlignVCenter);
		titleBandLayout->addLayout(titleStack, 1);

		QWidget* headerUtilities = CreateHeaderContextPanel(titleBand);
		if (headerUtilities != nullptr)
		{
			titleBandLayout->addWidget(headerUtilities, 0, Qt::AlignRight | Qt::AlignVCenter);
		}
		layout->addWidget(titleBand, 0);

		if (m_operationStack != nullptr)
		{
			m_operationStack->setParent(panel);
			layout->addWidget(m_operationStack, 0);
		}

		m_optionsStack = new QStackedWidget(panel);
		m_optionsStack->setObjectName("OptionsStack");
		RebuildOptionsPages();
		layout->addWidget(m_optionsStack, 1);
		m_optionsStack->setVisible(false);

		m_actionMetaPanel = new QFrame(panel);
		m_actionMetaPanel->setObjectName("ActionMetaPanel");
		QHBoxLayout* actionMetaRowLayout = new QHBoxLayout(m_actionMetaPanel);
		actionMetaRowLayout->setContentsMargins(LauncherUi::ActionMeta::Margins);
		actionMetaRowLayout->setSpacing(LauncherUi::ActionMeta::Spacing);
		actionMetaRowLayout->addStretch(1);

		m_cleanButton = new QPushButton("Clean", panel);
		m_cleanButton->setObjectName("SecondaryButton");
		m_cleanButton->setFixedSize(LauncherUi::ActionMeta::SecondaryButtonWidth, LauncherUi::ActionMeta::ButtonHeight);
		m_cleanButton->setToolTip("Clean only the generated outputs tied to this action.");
		m_cleanButton->setEnabled(false);
		m_cleanButton->setAccessibleName("Clean selected workflow outputs");
		RegisterFocusable(m_cleanButton);
		connect(m_cleanButton, &QPushButton::clicked, this, &LauncherMainWindow::CleanSelectedOperation);
		actionMetaRowLayout->addWidget(m_cleanButton, 0, Qt::AlignRight | Qt::AlignVCenter);

		m_runButton = new QPushButton("Run", panel);
		m_runButton->setObjectName("PrimaryActionButton");
		m_runButton->setFixedSize(LauncherUi::ActionMeta::PrimaryButtonWidth, LauncherUi::ActionMeta::ButtonHeight);
		m_runButton->setIcon(m_icons.Icon(LauncherIcon::Run, QColor("#ffffff")));
		m_runButton->setIconSize(QSize(kLauncherIconSize, kLauncherIconSize));
		m_runButton->setToolTip("Run the selected workflow. Existing runs keep going.");
		m_runButton->setEnabled(false);
		m_runButton->setAccessibleName("Run selected workflow");
		RegisterFocusable(m_runButton);
		connect(m_runButton, &QPushButton::clicked, this, &LauncherMainWindow::RunSelectedOperation);
		actionMetaRowLayout->addWidget(m_runButton, 0, Qt::AlignRight | Qt::AlignVCenter);
		layout->addWidget(m_actionMetaPanel);
		return panel;
	}

	QWidget* LauncherMainWindow::CreateHeaderContextPanel(QWidget* parent)
	{
		QFrame* panel = new QFrame(parent);
		panel->setObjectName("HeaderUtilityPanel");
		QHBoxLayout* rowLayout = new QHBoxLayout(panel);
		rowLayout->setContentsMargins(0, 0, 0, 0);
		rowLayout->setSpacing(LauncherUi::HeaderContext::Spacing);

		QLabel* levelLabel = CreateFieldLabel("Level");
		levelLabel->setObjectName("HeaderFieldLabel");
		rowLayout->addWidget(levelLabel, 0);
		QComboBox* levelCombo = CreateStartupLevelCombo();
		levelCombo->setObjectName("HeaderContextCombo");
		levelCombo->setAccessibleName("Startup level");
		levelCombo->setToolTip("Startup level used by editor and runtime launches.");
		ApplyContextComboMetrics(*levelCombo, LauncherUi::HeaderContext::LevelComboMinWidth, LauncherUi::HeaderContext::LevelComboMaxWidth);
		levelLabel->setBuddy(levelCombo);
		rowLayout->addWidget(levelCombo, 0);

		QLabel* graphicsApiLabel = CreateFieldLabel("Graphics API");
		graphicsApiLabel->setObjectName("HeaderFieldLabel");
		rowLayout->addWidget(graphicsApiLabel, 0);
		m_graphicsApiCombo = CreateContextCombo(&LauncherSettings::SetGraphicsApi);
		m_graphicsApiCombo->setObjectName("HeaderContextCombo");
		m_graphicsApiCombo->setAccessibleName("Graphics API");
		m_graphicsApiCombo->setToolTip(
		    "Graphics API used by Quick Start. Available backends are selectable; supported backends that need setup are shown disabled.");
		ApplyContextComboMetrics(
		    *m_graphicsApiCombo,
		    LauncherUi::HeaderContext::GraphicsApiComboMinWidth,
		    LauncherUi::HeaderContext::GraphicsApiComboMaxWidth);
		graphicsApiLabel->setBuddy(m_graphicsApiCombo);
		rowLayout->addWidget(m_graphicsApiCombo, 0);

		return panel;
	}

	QWidget* LauncherMainWindow::CreateFooterContextPanel(QWidget* parent)
	{
		QFrame* panel = new QFrame(parent);
		panel->setObjectName("FooterContextPanel");
		QHBoxLayout* rowLayout = new QHBoxLayout(panel);
		rowLayout->setContentsMargins(LauncherUi::FooterContext::Margins);
		rowLayout->setSpacing(LauncherUi::FooterContext::Spacing);
		rowLayout->addStretch(1);

		const auto addLabel = [this, rowLayout](const QString& text)
		{
			QLabel* label = CreateFieldLabel(text);
			label->setObjectName("FooterFieldLabel");
			rowLayout->addWidget(label, 0);
			return label;
		};
		const auto finishCombo = [rowLayout](QLabel& label, QComboBox& combo, int minWidth, int maxWidth)
		{
			combo.setObjectName("FooterContextCombo");
			ApplyContextComboMetrics(combo, minWidth, maxWidth);
			label.setBuddy(&combo);
			rowLayout->addWidget(&combo, 0);
		};

		QLabel* configurationLabel = addLabel("Config");
		m_buildConfigurationCombo = CreateContextCombo(&LauncherSettings::SetBuildConfiguration);
		m_buildConfigurationCombo->setAccessibleName("Build Configuration");
		m_buildConfigurationCombo->setToolTip(
		    "Global editor and runtime configuration. Only configurations backed by both product profiles are selectable.");
		finishCombo(
		    *configurationLabel,
		    *m_buildConfigurationCombo,
		    LauncherUi::HeaderContext::ConfigurationComboMinWidth,
		    LauncherUi::HeaderContext::ConfigurationComboMaxWidth);

		QLabel* compilerLabel = addLabel("Compiler");
		m_workspaceCompilerCombo = CreateContextCombo(&LauncherSettings::SetWorkspaceCompiler);
		m_workspaceCompilerCombo->setAccessibleName("Compiler");
		m_workspaceCompilerCombo->setToolTip(
		    "Compiler configured by the launcher. Installed compilers are selectable; supported missing compilers remain visible for "
		    "setup.");
		finishCombo(
		    *compilerLabel,
		    *m_workspaceCompilerCombo,
		    LauncherUi::HeaderContext::CompilerComboMinWidth,
		    LauncherUi::HeaderContext::CompilerComboMaxWidth);

		QLabel* ideLabel = addLabel("IDE");
		m_workspaceIdeCombo = CreateContextCombo(&LauncherSettings::SetWorkspaceIde);
		m_workspaceIdeCombo->setAccessibleName("IDE");
		m_workspaceIdeCombo->setToolTip(
		    "IDE opened by Quick Start. Detected IDEs are selectable; supported missing IDEs remain visible for setup.");
		finishCombo(
		    *ideLabel,
		    *m_workspaceIdeCombo,
		    LauncherUi::HeaderContext::IdeComboMinWidth,
		    LauncherUi::HeaderContext::IdeComboMaxWidth);
		return panel;
	}

	QComboBox* LauncherMainWindow::CreateContextCombo(void (LauncherSettings::*setter)(const QString&))
	{
		QComboBox* combo = new QComboBox(this);
		RegisterFocusable(combo);
		connect(
		    combo,
		    static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
		    [combo, setter, this]()
		    {
			    const QString value = combo->currentData().toString();
			    if (!value.isEmpty())
			    {
				    (m_settings.*setter)(value);
			    }
		    });
		return combo;
	}

	void LauncherMainWindow::RefreshContextSelectors()
	{
		const BuildToolchainStatus toolchain =
		    DetectBuildToolchain(m_repositoryRoot, ResolveSelectedWorkspaceIde(m_settings), ResolveSelectedWorkspaceCompiler(m_settings));
		const LauncherContextUiModel model = LauncherContextUiModel::Build(toolchain);
		PopulateBoundContextCombo(
		    m_graphicsApiCombo,
		    model.GraphicsApis,
		    m_settings.GraphicsApi(),
		    m_settings,
		    &LauncherSettings::SetGraphicsApi);
		PopulateBoundContextCombo(
		    m_buildConfigurationCombo,
		    model.BuildConfigurations,
		    m_settings.BuildConfiguration(),
		    m_settings,
		    &LauncherSettings::SetBuildConfiguration);
		PopulateBoundContextCombo(
		    m_workspaceCompilerCombo,
		    model.Compilers,
		    m_settings.WorkspaceCompiler(),
		    m_settings,
		    &LauncherSettings::SetWorkspaceCompiler);
		PopulateBoundContextCombo(
		    m_workspaceIdeCombo,
		    model.Ides,
		    m_settings.WorkspaceIde(),
		    m_settings,
		    &LauncherSettings::SetWorkspaceIde);
	}

	QWidget* LauncherMainWindow::CreateOptionsPage(const QString& operationId, QWidget* parent)
	{
		QScrollArea* scrollArea = new QScrollArea(parent);
		scrollArea->setObjectName("OptionsScrollArea");
		scrollArea->setWidgetResizable(true);
		scrollArea->setFrameShape(QFrame::NoFrame);
		scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

		QWidget* content = new QWidget(scrollArea);
		content->setObjectName("OptionsContent");
		const bool isQuickStart = operationId == LauncherHomeOperationId();
		const bool isLevelCatalog = operationId == "workspace.sync-levels";
		scrollArea->setAlignment(isQuickStart ? Qt::AlignTop : (Qt::AlignLeft | Qt::AlignTop));
		content->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
		if (!isQuickStart && !isLevelCatalog)
		{
			content->setMaximumWidth(LauncherUi::Page::MaxContentWidth);
		}
		QVBoxLayout* layout = new QVBoxLayout(content);
		layout->setContentsMargins(isQuickStart ? LauncherUi::Page::QuickStartMargins : LauncherUi::Page::ContentMargins);
		layout->setSpacing(isQuickStart ? 0 : LauncherUi::Page::Spacing);
		AddOptionsForOperation(*layout, operationId);
		layout->addStretch(1);
		scrollArea->setWidget(content);
		return scrollArea;
	}

	QWidget* LauncherMainWindow::CreateOutputPanel()
	{
		const LauncherOutputPanelWidgets widgets = CreateLauncherOutputPanel(
		    this,
		    m_icons.Icon(LauncherIcon::Copy, QColor(kColorStateQueued)),
		    QSize(kLauncherIconSize, kLauncherIconSize),
		    [this](QWidget* widget) { RegisterFocusable(widget); },
		    [this]() { ToggleActivityLogPanel(); },
		    [this]() { CopySelectedRunOutput(); },
		    [this](QListWidgetItem* current, QListWidgetItem* previous) { DisplaySelectedRunOutput(current, previous); });

		if (widgets.Root != nullptr)
		{
			widgets.Root->setObjectName("ActivityBottomPanel");
			widgets.Root->setMinimumHeight(kActivityPanelCollapsedHeight);
			widgets.Root->setMaximumHeight(kActivityPanelCollapsedHeight);
		}
		m_activityPanel = widgets.Root;
		m_activityDetailsPanel = widgets.ActivityDetailsPanel;
		m_activityList = widgets.ActivityList;
		m_selectedRunSummary = widgets.SelectedRunSummary;
		m_operationOutput = widgets.OperationOutput;
		m_toggleOutputButton = widgets.ToggleOutputButton;
		m_copyOutputButton = widgets.CopyOutputButton;
		if (m_operationOutput != nullptr)
		{
			m_operationOutput->setMinimumHeight(kOperationOutputMinHeight);
			m_operationOutput->setMaximumHeight(kOperationOutputMaxHeight);
		}
		SetActivityLogExpanded(false);
		return widgets.Root;
	}

	QLabel* LauncherMainWindow::CreateSectionLabel(const QString& title) const
	{
		QLabel* label = new QLabel(title);
		label->setObjectName("SectionLabel");
		label->setAccessibleName(title);
		return label;
	}

	QLabel* LauncherMainWindow::CreateFieldLabel(const QString& title) const
	{
		QLabel* label = new QLabel(title);
		label->setObjectName("FieldLabel");
		label->setAccessibleName(title);
		return label;
	}

	QCheckBox* LauncherMainWindow::CreateBoundCheckBox(
	    const QString& label,
	    const QString& tooltip,
	    bool checked,
	    void (LauncherSettings::*setter)(bool))
	{
		QCheckBox* box = new QCheckBox(label, this);
		box->setToolTip(tooltip);
		box->setAccessibleName(label);
		box->setAccessibleDescription(tooltip);
		box->setChecked(checked);
		RegisterFocusable(box);
		connect(box, &QCheckBox::toggled, &m_settings, setter);
		return box;
	}

	QLineEdit* LauncherMainWindow::CreateBoundLineEdit(
	    const QString& text,
	    const QString& placeholder,
	    const QString& tooltip,
	    void (LauncherSettings::*setter)(const QString&))
	{
		QLineEdit* edit = new QLineEdit(this);
		edit->setText(text);
		edit->setPlaceholderText(placeholder);
		edit->setToolTip(tooltip);
		edit->setAccessibleDescription(tooltip);
		RegisterFocusable(edit);
		connect(edit, &QLineEdit::textChanged, &m_settings, setter);
		return edit;
	}

	QTextEdit* LauncherMainWindow::CreateBoundTextEdit(
	    const QString& text,
	    const QString& placeholder,
	    const QString& tooltip,
	    void (LauncherSettings::*setter)(const QString&))
	{
		QTextEdit* edit = new QTextEdit(this);
		edit->setPlainText(text);
		edit->setPlaceholderText(placeholder);
		edit->setToolTip(tooltip);
		edit->setAccessibleDescription(tooltip);
		edit->setMinimumHeight(LauncherUi::TextEdit::MinHeight);
		edit->setMaximumHeight(LauncherUi::TextEdit::MaxHeight);
		RegisterFocusable(edit);
		connect(edit, &QTextEdit::textChanged, this, [edit, setter, this]() { (m_settings.*setter)(edit->toPlainText()); });
		return edit;
	}

	QComboBox* LauncherMainWindow::CreateProfileCombo(
	    const QStringList& profiles,
	    const QString& currentProfile,
	    void (LauncherSettings::*setter)(const QString&))
	{
		QComboBox* combo = new QComboBox(this);
		combo->addItems(profiles);
		combo->setAccessibleName("Profile");
		combo->setAccessibleDescription("Build profile used by this workflow.");
		combo->setCurrentText(currentProfile);
		RegisterFocusable(combo);
		connect(combo, &QComboBox::currentTextChanged, &m_settings, setter);
		return combo;
	}

	QComboBox* LauncherMainWindow::CreateValueCombo(
	    const QVector<QPair<QString, QString>>& options,
	    const QString& currentValue,
	    void (LauncherSettings::*setter)(const QString&))
	{
		QComboBox* combo = new QComboBox(this);
		combo->setAccessibleName("Option value");
		RegisterFocusable(combo);
		for (const QPair<QString, QString>& option : options)
		{
			combo->addItem(option.first, option.second);
		}
		const int currentIndex = combo->findData(currentValue);
		combo->setCurrentIndex(currentIndex >= 0 ? currentIndex : 0);
		connect(
		    combo,
		    static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
		    [combo, setter, this]() { (m_settings.*setter)(combo->currentData().toString()); });
		return combo;
	}

}
