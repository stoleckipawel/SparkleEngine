#include "LauncherMainWindow.h"

#include "LauncherActionWidgets.h"
#include "LauncherCleanUiModel.h"
#include "LauncherDependencyUiModel.h"
#include "LauncherLayoutWidgets.h"
#include "LauncherOperationRequestFactory.h"
#include "LauncherPageUtilities.h"
#include "LauncherContentModel.h"
#include "LauncherSettings.h"
#include "LauncherToolchainUiModel.h"
#include "LauncherUiDesign.h"
#include "LauncherUiModel.h"
#include "LauncherWorkflowCatalog.h"

#include "SparkleLauncher/BuildWorkspaceOperations.h"
#include "SparkleLauncher/CookOperations.h"
#include "SparkleLauncher/LauncherPaths.h"
#include "SparkleLauncher/MaintenanceOperations.h"

#include <QtCore/QRegularExpression>
#include <QtCore/QSignalBlocker>
#include <QtCore/QStringList>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QFrame>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

#include <array>
#include <cstdint>
#include <filesystem>
#include <system_error>
#include <vector>

namespace SparkleLauncher
{
	static constexpr int kFieldLabelWidth = LauncherUi::Row::FieldLabelWidth;

	bool LauncherMainWindow::UsesBuildEnvironmentStatus(const QString& operationId)
	{
		return operationId == "workspace.generate-build-files" || operationId == "workspace.sync-code" || operationId == "workspace.build"
		    || operationId == "workspace.build.editor" || operationId == "launcher.build.self" || operationId == "workspace.build.runtime"
		    || operationId.startsWith("cook.");
	}

	void LauncherMainWindow::AddOptionsForOperation(QVBoxLayout& layout, const QString& operationId)
	{
		if (operationId == LauncherHomeOperationId())
		{
			AddHomeQuickStart(layout);
			return;
		}

		if (operationId == "workspace.build")
		{
			AddBuildOptions(layout);
			return;
		}

		if (operationId == "cook.workspace")
		{
			AddCookOptions(layout);
			return;
		}

		if (UsesBuildEnvironmentStatus(operationId))
		{
			AddBuildEnvironmentStatus(layout, operationId);
			return;
		}

		if (operationId == "workspace.clean")
		{
			AddCleanOptions(layout, operationId);
			return;
		}

		AddNoOptionsMessage(layout, "No settings");
	}

	void LauncherMainWindow::AddBuildOptions(QVBoxLayout& layout)
	{
		const WorkspaceFeatureSettings features = GetLauncherWorkspaceFeatureSettings();
		const bool cookToolsAvailable = features.ContentPipelineEnabled || features.ShaderCompilerEnabled;
		const QStringList selectedScopes = m_settings.BuildScopes().split(QRegularExpression("[,;\\n]"), Qt::SkipEmptyParts);
		QVector<QCheckBox*> scopeBoxes;

		QVBoxLayout* buildLayout =
		    AddOptionGroup(layout, "Choose products", "Select the products to build. CMake resolves their shared dependencies.");

		QLabel* selectionSummary = new QLabel(buildLayout->parentWidget());
		selectionSummary->setObjectName("WorkflowSelectionSummary");
		buildLayout->addWidget(selectionSummary);

		QFrame* selectionPanel = new QFrame(buildLayout->parentWidget());
		selectionPanel->setObjectName("WorkflowSelectionPanel");
		selectionPanel->setMinimumWidth(LauncherUi::ScopeSelection::ContentMinWidth);
		selectionPanel->setMaximumWidth(LauncherUi::ScopeSelection::ContentMaxWidth);
		QVBoxLayout* selectionLayout = new QVBoxLayout(selectionPanel);
		selectionLayout->setContentsMargins(0, 0, 0, 0);
		selectionLayout->setSpacing(0);

		AddWorkflowScopeRow(
		    *selectionLayout,
		    "Editor",
		    "editor",
		    "Authoring, inspection, and developer tools.",
		    m_settings.EditorProfile(),
		    true,
		    selectedScopes,
		    scopeBoxes);
		AddWorkflowScopeRow(
		    *selectionLayout,
		    "Game",
		    "runtime",
		    "Standalone runtime used by Game mode.",
		    m_settings.RuntimeProfile(),
		    true,
		    selectedScopes,
		    scopeBoxes);
		AddWorkflowScopeRow(
		    *selectionLayout,
		    "Cooking tools",
		    "cook-tools",
		    cookToolsAvailable ? "Asset, texture, and shader cooking tools."
		                       : "No cooking-tool targets are enabled by this workspace configuration.",
		    cookToolsAvailable ? m_settings.EditorProfile() : QStringLiteral("Unavailable"),
		    cookToolsAvailable,
		    selectedScopes,
		    scopeBoxes);
		AddWorkflowScopeRow(
		    *selectionLayout,
		    "Launcher",
		    "launcher",
		    "Sparkle Launcher; the new build is used after restart.",
		    m_settings.EditorProfile(),
		    true,
		    selectedScopes,
		    scopeBoxes);
		buildLayout->addWidget(selectionPanel);

		AddWorkflowAutomationNote(*buildLayout, "CMake refreshes stale build files and skips current targets.");

		for (QCheckBox* scopeBox : scopeBoxes)
		{
			connect(
			    scopeBox,
			    &QCheckBox::toggled,
			    this,
			    [this, scopeBoxes, selectionSummary]() { UpdateBuildScopeSetting(scopeBoxes, selectionSummary); });
		}
		UpdateBuildScopeSetting(scopeBoxes, selectionSummary);
		AddBuildEnvironmentStatus(layout, "workspace.build");
	}

	void LauncherMainWindow::AddWorkflowScopeRow(
	    QVBoxLayout& layout,
	    const QString& label,
	    const QString& value,
	    const QString& detail,
	    const QString& metadata,
	    bool available,
	    const QStringList& selectedScopes,
	    QVector<QCheckBox*>& scopeBoxes)
	{
		QFrame* scopeRow = new QFrame(layout.parentWidget());
		scopeRow->setObjectName("WorkflowScopeRow");
		QHBoxLayout* scopeRowLayout = new QHBoxLayout(scopeRow);
		scopeRowLayout->setContentsMargins(LauncherUi::ScopeSelection::RowMargins);
		scopeRowLayout->setSpacing(LauncherUi::ScopeSelection::RowSpacing);

		QVBoxLayout* descriptionLayout = new QVBoxLayout();
		descriptionLayout->setContentsMargins(0, 0, 0, 0);
		descriptionLayout->setSpacing(2);
		QCheckBox* scopeBox = new QCheckBox(label, scopeRow);
		scopeBox->setObjectName("WorkflowScopeCheckBox");
		scopeBox->setProperty("ScopeValue", value);
		scopeBox->setProperty("ScopeLabel", label);
		scopeBox->setToolTip(detail);
		scopeBox->setChecked(available && selectedScopes.contains(value));
		scopeBox->setEnabled(available);
		RegisterFocusable(scopeBox);
		descriptionLayout->addWidget(scopeBox);

		QLabel* scopeDescription = new QLabel(detail, scopeRow);
		scopeDescription->setObjectName("WorkflowScopeDescription");
		scopeDescription->setWordWrap(true);
		descriptionLayout->addWidget(scopeDescription);
		scopeRowLayout->addLayout(descriptionLayout, 1);

		QLabel* scopeMetadata = new QLabel(metadata, scopeRow);
		scopeMetadata->setObjectName("WorkflowScopeMetadata");
		scopeMetadata->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
		scopeRowLayout->addWidget(scopeMetadata, 0);

		scopeRow->setProperty("Selected", scopeBox->isChecked());
		connect(
		    scopeBox,
		    &QCheckBox::toggled,
		    scopeRow,
		    [scopeRow](bool selected)
		    {
			    scopeRow->setProperty("Selected", selected);
			    scopeRow->style()->unpolish(scopeRow);
			    scopeRow->style()->polish(scopeRow);
		    });

		layout.addWidget(scopeRow);
		scopeBoxes.push_back(scopeBox);
	}

	void LauncherMainWindow::AddWorkflowAutomationNote(QVBoxLayout& layout, const QString& detail)
	{
		QFrame* automationNote = new QFrame(layout.parentWidget());
		automationNote->setObjectName("WorkflowAutomationNote");
		automationNote->setMinimumWidth(LauncherUi::ScopeSelection::ContentMinWidth);
		automationNote->setMaximumWidth(LauncherUi::ScopeSelection::ContentMaxWidth);
		QHBoxLayout* automationLayout = new QHBoxLayout(automationNote);
		automationLayout->setContentsMargins(LauncherUi::ScopeSelection::RowMargins);
		automationLayout->setSpacing(LauncherUi::ScopeSelection::RowSpacing);
		QLabel* automationTitle = new QLabel("AUTOMATIC", automationNote);
		automationTitle->setObjectName("WorkflowAutomationTitle");
		automationLayout->addWidget(automationTitle, 0, Qt::AlignTop);
		QLabel* automationDetail = new QLabel(detail, automationNote);
		automationDetail->setObjectName("WorkflowAutomationDetail");
		automationDetail->setWordWrap(true);
		automationLayout->addWidget(automationDetail, 1);
		layout.addWidget(automationNote);
	}

	void LauncherMainWindow::UpdateBuildScopeSetting(const QVector<QCheckBox*>& scopeBoxes, QLabel* selectionSummary)
	{
		QStringList selectedValues;
		QStringList selectedLabels;
		for (QCheckBox* scopeBox : scopeBoxes)
		{
			if (scopeBox != nullptr && scopeBox->isEnabled() && scopeBox->isChecked())
			{
				selectedValues.push_back(scopeBox->property("ScopeValue").toString());
				selectedLabels.push_back(scopeBox->property("ScopeLabel").toString());
			}
		}

		if (selectionSummary != nullptr)
		{
			selectionSummary->setText(QStringLiteral("Select at least one product to build."));
			selectionSummary->setProperty("State", selectedLabels.empty() ? "warning" : "ok");
			selectionSummary->setVisible(selectedLabels.empty());
			selectionSummary->style()->unpolish(selectionSummary);
			selectionSummary->style()->polish(selectionSummary);
		}
		m_settings.SetBuildScopes(selectedValues.join(';'));
		UpdateRunAvailability();
	}

	void LauncherMainWindow::AddCookOptions(QVBoxLayout& layout)
	{
		const WorkspaceFeatureSettings features = GetLauncherWorkspaceFeatureSettings();
		const QStringList selectedScopes = m_settings.CookScopes().split(QRegularExpression("[,;\\n]"), Qt::SkipEmptyParts);
		QVector<QCheckBox*> scopeBoxes;

		QVBoxLayout* cookLayout = AddOptionGroup(layout, "Choose outputs", "Select the runtime content to prepare.");

		QLabel* selectionSummary = new QLabel(cookLayout->parentWidget());
		selectionSummary->setObjectName("WorkflowSelectionSummary");
		cookLayout->addWidget(selectionSummary);

		QFrame* selectionPanel = new QFrame(cookLayout->parentWidget());
		selectionPanel->setObjectName("WorkflowSelectionPanel");
		selectionPanel->setMinimumWidth(LauncherUi::ScopeSelection::ContentMinWidth);
		selectionPanel->setMaximumWidth(LauncherUi::ScopeSelection::ContentMaxWidth);
		QVBoxLayout* selectionLayout = new QVBoxLayout(selectionPanel);
		selectionLayout->setContentsMargins(0, 0, 0, 0);
		selectionLayout->setSpacing(0);

		AddWorkflowScopeRow(
		    *selectionLayout,
		    "Shaders",
		    "shaders",
		    features.ShaderCompilerEnabled ? "Compile every registered shader into the cooked map and code library."
		                                   : "Shader cooking is not enabled by this workspace configuration.",
		    features.ShaderCompilerEnabled ? QStringLiteral("DXIL + SPIR-V") : QStringLiteral("Unavailable"),
		    features.ShaderCompilerEnabled,
		    selectedScopes,
		    scopeBoxes);
		AddWorkflowScopeRow(
		    *selectionLayout,
		    "Textures",
		    "textures",
		    features.ContentPipelineEnabled ? "Create runtime-ready texture products."
		                                    : "Texture cooking is not enabled by this workspace configuration.",
		    features.ContentPipelineEnabled ? QStringLiteral("Asset + Texture cooker") : QStringLiteral("Unavailable"),
		    features.ContentPipelineEnabled,
		    selectedScopes,
		    scopeBoxes);
		AddWorkflowScopeRow(
		    *selectionLayout,
		    "Scene assets",
		    "assets",
		    features.ContentPipelineEnabled ? "Cook scenes, meshes, and materials."
		                                    : "Scene-asset cooking is not enabled by this workspace configuration.",
		    features.ContentPipelineEnabled ? QStringLiteral("Asset cooker") : QStringLiteral("Unavailable"),
		    features.ContentPipelineEnabled,
		    selectedScopes,
		    scopeBoxes);
		cookLayout->addWidget(selectionPanel);

		AddWorkflowAutomationNote(
		    *cookLayout,
		    selectedScopes.contains("shaders")
		        ? QStringLiteral(
		              "ShaderCompiler discovers typed shaders and canonical targets. Incremental cooking preserves unaffected map entries.")
		        : QStringLiteral("Incremental cooking reuses current outputs."));

		for (QCheckBox* scopeBox : scopeBoxes)
		{
			connect(
			    scopeBox,
			    &QCheckBox::toggled,
			    this,
			    [this, scopeBoxes, selectionSummary]() { UpdateCookScopeSetting(scopeBoxes, selectionSummary); });
		}
		UpdateCookScopeSetting(scopeBoxes, selectionSummary);

		AddBuildEnvironmentStatus(layout, "cook.workspace");
	}

	void LauncherMainWindow::UpdateCookScopeSetting(const QVector<QCheckBox*>& scopeBoxes, QLabel* selectionSummary)
	{
		QStringList selectedValues;
		QStringList selectedLabels;
		for (QCheckBox* scopeBox : scopeBoxes)
		{
			if (scopeBox != nullptr && scopeBox->isEnabled() && scopeBox->isChecked())
			{
				selectedValues.push_back(scopeBox->property("ScopeValue").toString());
				selectedLabels.push_back(scopeBox->property("ScopeLabel").toString());
			}
		}

		if (selectionSummary != nullptr)
		{
			selectionSummary->setText(QStringLiteral("Select at least one output to cook."));
			selectionSummary->setProperty("State", selectedLabels.empty() ? "warning" : "ok");
			selectionSummary->setVisible(selectedLabels.empty());
			selectionSummary->style()->unpolish(selectionSummary);
			selectionSummary->style()->polish(selectionSummary);
		}
		m_settings.SetCookScopes(selectedValues.join(';'));
		UpdateRunAvailability();
	}

	void LauncherMainWindow::AddCleanOptions(QVBoxLayout& layout, const QString&)
	{
		const std::array<CleanScopeUiOption, 6> cleanScopes{{
		    {"Cooked content", "cooked", "Remove generated cooked assets for this workspace.", QString(), "Content outputs"},
		    {"Build workspace",
		        "build-tree",
		        "Remove build intermediates and generated CMake or IDE files; keep downloaded dependencies.",
		        "build content except build/_deps and generated workspace files",
		        "Build outputs"},
		    {"Generated artifacts",
		        "artifacts",
		        "Remove executables, libraries, symbols, diagnostics, and all cooked content.",
		        QString(),
		        "Build outputs"},
		    {"IDE and workspace state",
		        "workspace-state",
		        "Reset local Visual Studio, Rider, VS Code, and ImGui workspace state.",
		        ".vs, .vscode, .idea, and ImGui workspace state",
		        "Local state and caches"},
		    {"Source dependency cache",
		        "deps",
		        "Remove downloaded code dependencies; the next sync or configure downloads them again.",
		        QString(),
		        "Local state and caches"},
		    {"Logs",
		        "logs",
		        "Remove repository, launcher, and content diagnostic logs; keep the current launcher log until exit.",
		        "repository, launcher, and content logs",
		        "Local state and caches"},
		}};
		const std::array<QString, 3> cleanGroups{{
		    "Content outputs",
		    "Build outputs",
		    "Local state and caches",
		}};

		QVector<QCheckBox*> scopeBoxes;
		const QString contentId = m_contentModel.ContentId();
		QStringList selectedScopes = m_settings.CleanScope().split(QRegularExpression("[,;\\n]"), Qt::SkipEmptyParts);
		QVBoxLayout* cleanLayout = AddOptionGroup(
		    layout,
		    "Choose generated data",
		    "Select only what should be regenerated. Project source files and synced levels are preserved. You will confirm before "
		    "cleaning.");

		QLabel* selectionSummary = nullptr;

		QFrame* selectionPanel = new QFrame(cleanLayout->parentWidget());
		selectionPanel->setObjectName("CleanSelectionPanel");
		selectionPanel->setMinimumWidth(LauncherUi::ScopeSelection::ContentMinWidth);
		selectionPanel->setMaximumWidth(LauncherUi::ScopeSelection::ContentMaxWidth);
		QVBoxLayout* selectionLayout = new QVBoxLayout(selectionPanel);
		selectionLayout->setContentsMargins(0, 0, 0, 0);
		selectionLayout->setSpacing(0);

		for (const QString& cleanGroup : cleanGroups)
		{
			QLabel* groupTitle = CreateSectionLabel(cleanGroup);
			groupTitle->setObjectName("CleanScopeGroupTitle");
			selectionLayout->addWidget(groupTitle);
			for (const CleanScopeUiOption& scope : cleanScopes)
			{
				if (scope.Group != cleanGroup)
				{
					continue;
				}

				AddCleanScopeRow(*selectionLayout, scope, contentId, selectedScopes, scopeBoxes);
			}
		}
		cleanLayout->addWidget(selectionPanel);

		for (QCheckBox* scopeBox : scopeBoxes)
		{
			connect(
			    scopeBox,
			    &QCheckBox::toggled,
			    this,
			    [this, scopeBoxes, selectionSummary, scopeBox]() { UpdateCleanScopeSetting(scopeBoxes, selectionSummary, scopeBox); });
		}

		UpdateCleanScopeSetting(scopeBoxes, selectionSummary);
	}

	void LauncherMainWindow::AddCleanScopeRow(
	    QVBoxLayout& layout,
	    const CleanScopeUiOption& scope,
	    const QString& contentId,
	    const QStringList& selectedScopes,
	    QVector<QCheckBox*>& scopeBoxes)
	{
		QFrame* scopeRow = new QFrame(layout.parentWidget());
		scopeRow->setObjectName("CleanScopeRow");
		QHBoxLayout* scopeRowLayout = new QHBoxLayout(scopeRow);
		scopeRowLayout->setContentsMargins(LauncherUi::Clean::RowMargins);
		scopeRowLayout->setSpacing(LauncherUi::Clean::RowSpacing);

		QVBoxLayout* descriptionLayout = new QVBoxLayout();
		descriptionLayout->setContentsMargins(0, 0, 0, 0);
		descriptionLayout->setSpacing(2);

		QCheckBox* scopeBox = new QCheckBox(scope.Label, scopeRow);
		scopeBox->setObjectName("CleanScopeCheckBox");
		scopeBox->setToolTip(scope.Detail);
		scopeBox->setProperty("CleanScope", scope.Value);
		scopeBox->setProperty("CleanLabel", scope.Label);
		scopeBox->setChecked(selectedScopes.contains(scope.Value) || (selectedScopes.empty() && scope.Value == "build-tree"));
		RegisterFocusable(scopeBox);
		descriptionLayout->addWidget(scopeBox);

		QLabel* scopeDescription = new QLabel(scope.Detail, scopeRow);
		scopeDescription->setObjectName("CleanScopeDescription");
		scopeDescription->setWordWrap(true);
		descriptionLayout->addWidget(scopeDescription);
		scopeRowLayout->addLayout(descriptionLayout, 3);

		const std::filesystem::path previewPath = ResolveCleanScopePreviewPath(m_repositoryRoot, contentId, scope.Value);
		const QString previewText = scope.Value == "cooked"
		    ? QStringLiteral("Generated content - ") + FormatDirectoryInventory(previewPath)
		    : (scope.Preview.isEmpty() ? ToDisplayPath(m_repositoryRoot, previewPath) + " - " + FormatDirectoryInventory(previewPath)
		                               : scope.Preview);
		ElidedLabel* scopeDetail = new ElidedLabel(previewText, scopeRow);
		scopeDetail->setObjectName("CleanScopePreview");
		scopeDetail->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
		scopeDetail->setMaximumWidth(380);
		scopeRowLayout->addWidget(scopeDetail, 2);

		scopeRow->setProperty("Selected", scopeBox->isChecked());
		connect(
		    scopeBox,
		    &QCheckBox::toggled,
		    scopeRow,
		    [scopeRow](bool selected)
		    {
			    scopeRow->setProperty("Selected", selected);
			    scopeRow->style()->unpolish(scopeRow);
			    scopeRow->style()->polish(scopeRow);
		    });

		layout.addWidget(scopeRow);
		scopeBoxes.push_back(scopeBox);
	}

	void LauncherMainWindow::UpdateCleanScopeSetting(
	    const QVector<QCheckBox*>& scopeBoxes,
	    QLabel* selectionSummary,
	    QCheckBox* changedScope)
	{
		const auto findScopeBox = [&scopeBoxes](const QString& scopeValue) -> QCheckBox*
		{
			for (QCheckBox* scopeBox : scopeBoxes)
			{
				if (scopeBox != nullptr && scopeBox->property("CleanScope").toString() == scopeValue)
				{
					return scopeBox;
				}
			}
			return nullptr;
		};
		const auto clearScope = [&findScopeBox](const QString& scopeValue)
		{
			if (QCheckBox* scopeBox = findScopeBox(scopeValue))
			{
				const QSignalBlocker blocker(scopeBox);
				scopeBox->setChecked(false);
				if (QWidget* scopeRow = scopeBox->parentWidget())
				{
					scopeRow->setProperty("Selected", false);
					scopeRow->style()->unpolish(scopeRow);
					scopeRow->style()->polish(scopeRow);
				}
			}
		};

		if (changedScope != nullptr && changedScope->isChecked())
		{
			const QString changedValue = changedScope->property("CleanScope").toString();
			if (changedValue == "artifacts")
			{
				clearScope("cooked");
			}
			else if (changedValue == "cooked")
			{
				clearScope("artifacts");
			}
		}
		else if (changedScope == nullptr)
		{
			if (QCheckBox* artifacts = findScopeBox("artifacts"); artifacts != nullptr && artifacts->isChecked())
			{
				clearScope("cooked");
			}
		}

		QStringList selectedValues;
		QStringList selectedLabels;
		for (QCheckBox* scopeBox : scopeBoxes)
		{
			if (scopeBox != nullptr && scopeBox->isChecked())
			{
				selectedValues.push_back(scopeBox->property("CleanScope").toString());
				selectedLabels.push_back(scopeBox->property("CleanLabel").toString());
			}
		}

		if (selectedValues.empty())
		{
			if (QCheckBox* buildWorkspace = findScopeBox("build-tree"))
			{
				const QSignalBlocker blocker(buildWorkspace);
				buildWorkspace->setChecked(true);
				if (QWidget* scopeRow = buildWorkspace->parentWidget())
				{
					scopeRow->setProperty("Selected", true);
					scopeRow->style()->unpolish(scopeRow);
					scopeRow->style()->polish(scopeRow);
				}
			}
			selectedValues.push_back("build-tree");
			selectedLabels.push_back("Build workspace");
		}

		if (selectionSummary != nullptr)
		{
			selectionSummary->setText(
			    selectedLabels.size() == 1 ? QStringLiteral("Selected: %1").arg(selectedLabels.front())
			                               : QStringLiteral("%1 categories selected").arg(selectedLabels.size()));
		}
		m_settings.SetCleanScope(selectedValues.join(';'));
		UpdateRunAvailability();
	}

	QWidget* LauncherMainWindow::AddOptionField(QVBoxLayout& layout, const QString& label, QWidget* control)
	{
		QFrame* row = new QFrame(this);
		row->setObjectName("OptionRow");
		QHBoxLayout* rowLayout = new QHBoxLayout(row);
		rowLayout->setContentsMargins(0, 0, 0, 0);
		rowLayout->setSpacing(0);

		QFrame* labelCell = new QFrame(row);
		labelCell->setObjectName("OptionLabelCell");
		QHBoxLayout* labelLayout = new QHBoxLayout(labelCell);
		labelLayout->setContentsMargins(LauncherUi::Option::LabelMargins);
		labelLayout->setSpacing(0);

		QLabel* fieldLabel = CreateFieldLabel(label);
		fieldLabel->setAlignment(Qt::AlignLeft | (qobject_cast<QTextEdit*>(control) != nullptr ? Qt::AlignTop : Qt::AlignVCenter));
		fieldLabel->setBuddy(control);
		labelLayout->addWidget(fieldLabel);
		labelCell->setFixedWidth(kFieldLabelWidth + 16);

		QFrame* valueCell = new QFrame(row);
		valueCell->setObjectName("OptionValueCell");
		valueCell->setFixedWidth(LauncherUi::Row::FieldValueWidth);
		QHBoxLayout* valueLayout = new QHBoxLayout(valueCell);
		valueLayout->setContentsMargins(0, 0, 0, 0);
		valueLayout->setSpacing(0);
		if (control->accessibleName().isEmpty() || control->accessibleName() == "Option value")
		{
			control->setAccessibleName(label);
		}
		if (control->toolTip().isEmpty())
		{
			control->setToolTip("Choose " + label.toLower() + " for this workflow.");
		}
		valueLayout->addWidget(control, 1);
		rowLayout->addWidget(labelCell, 0);
		rowLayout->addWidget(valueCell, 0);
		rowLayout->addStretch(1);
		layout.addWidget(row);
		return row;
	}

	QVBoxLayout* LauncherMainWindow::AddOptionGroup(QVBoxLayout& layout, const QString& title, const QString& detail)
	{
		QFrame* group = new QFrame(this);
		group->setObjectName("OptionGroup");
		QVBoxLayout* groupLayout = new QVBoxLayout(group);
		groupLayout->setContentsMargins(LauncherUi::Option::GroupMargins);
		groupLayout->setSpacing(LauncherUi::Option::GroupSpacing);

		QLabel* titleLabel = new QLabel(title, group);
		titleLabel->setObjectName("OptionGroupTitle");
		groupLayout->addWidget(titleLabel);

		if (!detail.isEmpty())
		{
			QLabel* detailLabel = new QLabel(detail, group);
			detailLabel->setObjectName("OptionHelpText");
			detailLabel->setWordWrap(true);
			groupLayout->addWidget(detailLabel);
		}

		layout.addWidget(group);
		return groupLayout;
	}

	QVBoxLayout* LauncherMainWindow::AddInlineOptionsSection(QVBoxLayout& layout)
	{
		QFrame* section = new QFrame(this);
		section->setObjectName("InlineOptionsSection");
		QVBoxLayout* sectionLayout = new QVBoxLayout(section);
		sectionLayout->setContentsMargins(0, 0, 0, 0);
		sectionLayout->setSpacing(LauncherUi::Section::Spacing);
		layout.addWidget(section);
		return sectionLayout;
	}

	void LauncherMainWindow::AddNoOptionsMessage(QVBoxLayout& layout, const QString& text)
	{
		QLabel* label = new QLabel(text, this);
		label->setObjectName("MutedLabel");
		label->setAccessibleName(text);
		label->setWordWrap(true);
		layout.addWidget(label);
	}

}
