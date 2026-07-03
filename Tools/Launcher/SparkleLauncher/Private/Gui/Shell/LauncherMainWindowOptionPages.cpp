#include "LauncherMainWindow.h"

#include "LauncherActionWidgets.h"
#include "LauncherArtworkWidgets.h"
#include "LauncherCleanUiModel.h"
#include "LauncherDependencyUiModel.h"
#include "LauncherHomeWidgets.h"
#include "LauncherLayoutWidgets.h"
#include "LauncherOperationRequestFactory.h"
#include "LauncherOutputWidgets.h"
#include "LauncherPageUtilities.h"
#include "LauncherProjectModel.h"
#include "LauncherRecoveryUiModel.h"
#include "LauncherSettings.h"
#include "LauncherToolchainUiModel.h"
#include "LauncherUiDesign.h"
#include "LauncherUiModel.h"
#include "LauncherWorkflowCatalog.h"

#include "SparkleLauncher/BuildWorkspaceOperations.h"
#include "SparkleLauncher/CookOperations.h"
#include "SparkleLauncher/LauncherPaths.h"
#include "SparkleLauncher/LaunchOperations.h"
#include "SparkleLauncher/MaintenanceOperations.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QProcess>
#include <QtCore/QRegularExpression>
#include <QtCore/QStringList>
#include <QtGui/QGuiApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QFrame>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QScrollArea>
#include <QtWidgets/QSizePolicy>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

#include <cstdint>
#include <filesystem>
#include <system_error>
#include <vector>

namespace SparkleLauncher
{
	static constexpr int kSpaceSmall = LauncherUi::Space::Small;
	static constexpr int kSpaceMedium = LauncherUi::Space::Medium;
	static constexpr int kFieldLabelWidth = LauncherUi::Row::FieldLabelWidth;
	static constexpr int kStatusChipColumnWidth = LauncherUi::Row::StatusChipColumnWidth;
	static constexpr int kStatusActionColumnWidth = LauncherUi::Row::StatusActionColumnWidth;
	static constexpr const char* kColorStateReady = LauncherUi::Color::StateSuccess;
	static constexpr const char* kColorStateWarning = LauncherUi::Color::StateWarning;	void LauncherMainWindow::AddOptionsForOperation(QVBoxLayout& layout, const QString& operationId)
	{
		if (operationId == LauncherHomeOperationId())
		{
			AddHomeQuickStart(layout);
			return;
		}

		AddWorkflowPageHeader(layout, operationId);
		if (operationId != "toolchain.check" && operationId != "workspace.sync-source-tiers")
		{
			AddWorkflowVisualBanner(layout, operationId);
		}

		if (operationId == "package.release")
		{
			QVBoxLayout* packageLayout = AddOptionGroup(
			    layout,
			    "Package Assembly",
			    "Assemble a release package layout while keeping final validation and publishing sign-off separate.");
			AddStatusRow(
			    *packageLayout,
			    "Release package",
			    "Assembly target",
			    "Build the sparkle_release_assembly CMake target to assemble launcher, editor/runtime, cooked content, manifests, checksums, notes, licenses, and a separate symbols archive under dist/releases/<version>.",
			    "neutral");
			AddStatusRow(
			    *packageLayout,
			    "Validation",
			    "Separate sign-off",
			    "This workflow assembles dist/ packages. Publish readiness still requires the final validation checklist and release report.",
			    "neutral");
			QVBoxLayout* contentsLayout = AddDetailsGroup(
			    layout,
			    "Selection Details",
			    "Package inclusion follows product ownership, visibility, binary type, declared dependencies, and package navigation rules.",
			    false);
			AddStatusRow(*contentsLayout, "Launcher", "Included", "Package-root SparkleLauncher.exe and runtime support files.", "neutral");
			AddStatusRow(*contentsLayout, "Selected project products", "Staged when present", "Editor/runtime binaries and cooked content are staged from the selected project's artifacts.", "neutral");
			AddStatusRow(*contentsLayout, "Manifests", "Generated", "Release, build, dependency, bundled-runtime, file hash, checksum, and notes outputs.", "neutral");
			AddStatusRow(*contentsLayout, "Symbols", "Separate archive", "Debug symbols stay outside user-facing runtime packages.", "neutral");
			return;
		}

		if (operationId == "workspace.generate-build-files" || operationId == "workspace.open-ide" || operationId == "toolchain.check" || operationId == "workspace.sync-source-tiers")
		{
			AddBuildEnvironmentStatus(layout, operationId);
			return;
		}

		if (operationId == "workspace.build-all")
		{
			AddBuildEnvironmentStatus(layout, operationId);
			return;
		}

		if (operationId == "project.build.editor")
		{
			AddBuildEnvironmentStatus(layout, operationId);
			return;
		}

		if (operationId == "launcher.build.self")
		{
			AddBuildEnvironmentStatus(layout, operationId);
			return;
		}

		if (operationId == "project.build.runtime")
		{
			AddBuildEnvironmentStatus(layout, operationId);
			return;
		}

		if (operationId == "cook.tools.prepare")
		{
			AddBuildEnvironmentStatus(layout, operationId);
			return;
		}

		if (operationId == "cook.shaders")
		{
			QVBoxLayout* selectionLayout = AddOptionGroup(layout, "Options", "Shader cook target selection. Advanced cache, debug, and compiler controls are available below.");
			AddOptionField(*selectionLayout, "Shader package", CreateValueCombo(
			    {{"All shader packages", ""},
			     {"ComputeClear", "ComputeClear"},
			     {"DirectLighting", "DirectLighting"},
			     {"GBuffer", "GBuffer"},
			     {"HelloInlineRayQuery", "HelloInlineRayQuery"},
			     {"HelloRayTracingLibrary", "HelloRayTracingLibrary"},
			     {"HelloTriangle", "HelloTriangle"},
			     {"IndirectLighting", "IndirectLighting"},
			     {"LightingComposite", "LightingComposite"},
			     {"Sky", "Sky"},
			     {"VisualizeBuffers", "VisualizeBuffers"}},
			    m_settings.ShaderPackages(),
			    &LauncherSettings::SetShaderPackages));
			AddOptionField(*selectionLayout, "Compiler backend", CreateValueCombo(
			    {{"Auto select", "auto"}, {"DXC", "dxc"}, {"Slang", "slang"}},
			    m_settings.ShaderBackend(),
			    &LauncherSettings::SetShaderBackend));
			QComboBox* targetPresetCombo = CreateValueCombo(
			    {{"Default runtime set (DxilSm66 + SpirV16)", "default"},
			     {"DirectX 12 only (DxilSm66)", "d3d12"},
			     {"Vulkan only (SpirV16)", "vulkan"},
			     {"DXIL compatibility sweep (Sm60-Sm67)", "dxil-all"},
			     {"SPIR-V compatibility sweep (1.4-1.6)", "spirv-all"},
			     {"Custom target list", "custom"}},
			    m_settings.ShaderTargetPreset(),
			    &LauncherSettings::SetShaderTargetPreset);
			AddOptionField(*selectionLayout, "Binary targets", targetPresetCombo);
			QWidget* customTargetsRow = AddOptionField(
			    *selectionLayout,
			    "Custom targets",
			    CreateBoundLineEdit(
			        m_settings.ShaderCustomTargets(),
			        "DxilSm66, SpirV16",
			        "Comma-separated ShaderCompiler target names such as DxilSm66 or SpirV16.",
			        &LauncherSettings::SetShaderCustomTargets));
			customTargetsRow->setVisible(m_settings.ShaderTargetPreset() == "custom");
			connect(targetPresetCombo, &QComboBox::currentTextChanged, customTargetsRow, [this, customTargetsRow](const QString&) {
				customTargetsRow->setVisible(m_settings.ShaderTargetPreset() == "custom");
			});

			QVBoxLayout* advancedLayout = AddDetailsGroup(layout, "Advanced Shader Options", "Cache, diagnostics, and compiler-output controls for shader investigation and production tuning.", false);
			advancedLayout->addWidget(CreateSectionLabel("Cache And Outputs"));
			AddOptionCheckBox(*advancedLayout, CreateBoundCheckBox("Use shader cache", "Reuse cached shader compile artifacts when possible.", m_settings.ShaderUseCache(), &LauncherSettings::SetShaderUseCache));
			AddOptionField(
			    *advancedLayout,
			    "Cache directory",
			    CreateBoundLineEdit(
			        m_settings.ShaderCacheDirectory(),
			        "Use ShaderCompiler default cache location",
			        "Optional override for ShaderCompiler --cache-dir.",
			        &LauncherSettings::SetShaderCacheDirectory));
			QCheckBox* debugArtifactsBox = CreateBoundCheckBox(
			    "Write debug artifact bundles",
			    "Emit compiler-side debug bundles and intermediate artifacts for inspection.",
			    m_settings.ShaderWriteDebugArtifacts(),
			    &LauncherSettings::SetShaderWriteDebugArtifacts);
			debugArtifactsBox->setObjectName("WarningCheckBox");
			AddOptionCheckBox(*advancedLayout, debugArtifactsBox);
			QWidget* debugArtifactsRow = AddOptionField(
			    *advancedLayout,
			    "Debug output directory",
			    CreateBoundLineEdit(
			        m_settings.ShaderDebugArtifactDirectory(),
			        ResolveShaderDebugArtifactDirectory(m_repositoryRoot, m_projectModel, m_settings),
			        "Optional override for ShaderCompiler --debug-artifacts. When empty, the launcher uses a build-local default directory.",
			        &LauncherSettings::SetShaderDebugArtifactDirectory));
			debugArtifactsRow->setVisible(m_settings.ShaderWriteDebugArtifacts());
			connect(debugArtifactsBox, &QCheckBox::toggled, debugArtifactsRow, &QWidget::setVisible);

			advancedLayout->addWidget(CreateSectionLabel("Diagnostics"));
			AddOptionCheckBox(
			    *advancedLayout,
			    CreateBoundCheckBox(
			        "Enable debug info and symbols",
			        "Request backend debug information and symbol emission where the selected backend supports it.",
			        m_settings.ShaderEnableDebugInfo(),
			        &LauncherSettings::SetShaderEnableDebugInfo));
			AddOptionCheckBox(
			    *advancedLayout,
			    CreateBoundCheckBox(
			        "Enable compiler optimizations",
			        "Compile shaders with backend optimizations enabled. Disable when debugging compiler output or reproducing optimization-sensitive issues.",
			        m_settings.ShaderEnableOptimizations(),
			        &LauncherSettings::SetShaderEnableOptimizations));
			AddOptionCheckBox(
			    *advancedLayout,
			    CreateBoundCheckBox(
			        "Treat warnings as errors",
			        "Fail the shader cook when the backend emits warnings.",
			        m_settings.ShaderWarningsAsErrors(),
			        &LauncherSettings::SetShaderWarningsAsErrors));
			AddOptionCheckBox(
			    *advancedLayout,
			    CreateBoundCheckBox(
			        "Strip reflection from runtime binaries",
			        "Request reflection stripping for final runtime shader binaries where the active backend supports it.",
			        m_settings.ShaderStripReflection(),
			        &LauncherSettings::SetShaderStripReflection));
			AddOptionCheckBox(
			    *advancedLayout,
			    CreateBoundCheckBox(
			        "Strip embedded debug info from runtime binaries",
			        "Request embedded debug info stripping for final runtime shader binaries where the active backend supports it.",
			        m_settings.ShaderStripDebugInfo(),
			        &LauncherSettings::SetShaderStripDebugInfo));
			AddOptionCheckBox(
			    *advancedLayout,
			    CreateBoundCheckBox(
			        "Write cooked shader stats CSV",
			        "Run the cooked-shader-stats analysis pass after the shader cook and write CSV output into the shader cache analysis folder.",
			        m_settings.ShaderWriteCookedShaderStats(),
			        &LauncherSettings::SetShaderWriteCookedShaderStats));
			AddBuildEnvironmentStatus(layout, operationId);
			return;
		}

		if (operationId.startsWith("cook."))
		{
			AddBuildEnvironmentStatus(layout, operationId);
			return;
		}

		if (operationId == "project.open.editor" || operationId == "project.open.runtime")
		{
			AddLaunchApplicationOptions(layout);
			AddLaunchEnvironmentStatus(layout, operationId);
			return;
		}

		if (operationId == "project.run")
		{
			AddLaunchTargetOptions(layout, "Launch Project", QString(), true);
			AddLaunchApplicationOptions(layout);
			AddLaunchEnvironmentStatus(layout, operationId);
			return;
		}

		if (operationId == "quality.format")
		{
			QVBoxLayout* formatOptionsLayout = AddOptionGroup(layout, "Formatting Mode", "Run clang-format as a quality gate or explicitly apply formatting changes.");
			AddOptionField(
			    *formatOptionsLayout,
			    "Mode",
			    CreateValueCombo(
			        {{"Check only", "check"}, {"Apply formatting", "apply"}},
			        m_settings.FormatMode(),
			        &LauncherSettings::SetFormatMode));
			AddMaintenanceEnvironmentStatus(layout, operationId);
			return;
		}

		if (operationId == "workspace.clean")
		{
			const std::array<CleanScopeUiOption, 9> cleanScopes = {{
			    {"Selected project cooked content", "selected-cooked", "Cooked asset outputs for only the selected project under artifacts/dev/projects/<Project>/cooked.", QString(), "Cooked content"},
			    {"All cooked content", "all-cooked", "Cooked asset domains for every project plus the shared cooked domain. Keeps editor/runtime artifacts and source dependency caches.", "artifacts/dev/projects/*/cooked", "Cooked content"},
			    {"Build outputs and generated build files", "build-tree", "Build outputs, intermediates, generated CMake/Visual Studio files, and project build trees. Keeps the source dependency cache.", "build content except build/_deps, root generated project files, project generated files", "Build and packages"},
			    {"Generated artifacts", "artifacts", "Runnable artifacts, libraries, symbols, diagnostics, and generated project outputs under artifacts/.", QString(), "Build and packages"},
			    {"Packaged outputs", "packages", "Release layouts and assembled package outputs under dist/.", "dist", "Build and packages"},
			    {"IDE and workspace state", "workspace-state", "Local IDE state and ImGui workspace state generated on this machine.", ".vs, .vscode, imgui.ini, Projects/*/imgui.ini", "Workspace state"},
			    {"Shader cache", "shader-cache", "Transient shader cache, recook signal, debug artifacts, and shader outputs.", QString(), "Caches"},
			    {"Source dependency cache", "deps", "Downloaded source dependency cache. Configure will re-download source dependency groups.", QString(), "Caches"},
			    {"Logs", "logs", "Repository, launcher, and project logs.", "logs, user-local launcher logs, Projects/*/logs", "Logs"},
			}};

			QVector<QCheckBox*> scopeBoxes;
			const QString selectedProjectId = m_projectModel.SelectedProjectId();
			const QStringList selectedScopes = m_settings.CleanScope().split(QRegularExpression("[,;\\n]"), Qt::SkipEmptyParts);
			const std::array<QPair<QString, QString>, 5> cleanGroups = {{
			    {"Cooked content", "Cooked assets"},
			    {"Build and packages", "Build products, artifacts, and packaged outputs"},
			    {"Workspace state", "IDE and local workspace state"},
			    {"Caches", "Regeneratable caches"},
			    {"Logs", "Launcher and project logs"},
			}};

			const auto addCleanScopeRow = [this, &scopeBoxes, &selectedProjectId, &selectedScopes](QGridLayout& groupGrid, const CleanScopeUiOption& scope, int row, int column) {
				QCheckBox* scopeBox = new QCheckBox(scope.Label, this);
				scopeBox->setToolTip(scope.Detail);
				scopeBox->setProperty("CleanScope", scope.Value);
				scopeBox->setProperty("CleanLabel", scope.Label);
				scopeBox->setChecked(selectedScopes.contains(scope.Value) || (selectedScopes.empty() && scope.Value == "build-tree"));
				RegisterFocusable(scopeBox);

				QFrame* scopeRow = new QFrame(this);
				scopeRow->setObjectName("CleanScopeCard");
				QVBoxLayout* scopeRowLayout = new QVBoxLayout(scopeRow);
				scopeRowLayout->setContentsMargins(LauncherUi::Clean::ScopeCardMargins());
				scopeRowLayout->setSpacing(LauncherUi::Clean::ScopeCardSpacing);
				scopeRowLayout->addWidget(scopeBox);
				const std::filesystem::path previewPath = ResolveCleanScopePreviewPath(m_repositoryRoot, selectedProjectId, scope.Value);
				const QString previewText = scope.Preview.isEmpty() ? ToDisplayPath(m_repositoryRoot, previewPath) + " - " + FormatDirectoryInventory(previewPath) : scope.Preview;
				QLabel* scopeDetail = new QLabel(previewText, scopeRow);
				scopeDetail->setObjectName("OptionHelpText");
				scopeDetail->setWordWrap(true);
				scopeRowLayout->addWidget(scopeDetail);
				groupGrid.addWidget(scopeRow, row, column);
				scopeBoxes.push_back(scopeBox);
			};

			QVBoxLayout* cleanScopesLayout = AddInlineOptionsSection(layout);
			for (const QPair<QString, QString>& cleanGroup : cleanGroups)
			{
				cleanScopesLayout->addWidget(CreateSectionLabel(cleanGroup.first));
				QGridLayout* cleanGrid = new QGridLayout();
				cleanGrid->setContentsMargins(LauncherUi::Clean::GridMargins());
				cleanGrid->setHorizontalSpacing(LauncherUi::Clean::GridSpacing);
				cleanGrid->setVerticalSpacing(LauncherUi::Clean::GridSpacing);
				int groupScopeIndex = 0;
				for (const CleanScopeUiOption& scope : cleanScopes)
				{
					if (scope.Group == cleanGroup.first)
					{
						addCleanScopeRow(*cleanGrid, scope, groupScopeIndex / 2, groupScopeIndex % 2);
						++groupScopeIndex;
					}
				}
				cleanScopesLayout->addLayout(cleanGrid);
			}

			const auto updateCleanScopeSetting = [scopeBoxes, this]() {
				QStringList selectedValues;
				for (QCheckBox* scopeBox : scopeBoxes)
				{
					if (scopeBox != nullptr && scopeBox->isChecked())
					{
						selectedValues.push_back(scopeBox->property("CleanScope").toString());
					}
				}
				if (selectedValues.empty())
				{
					if (!scopeBoxes.empty() && scopeBoxes.front() != nullptr)
					{
						const QSignalBlocker blocker(scopeBoxes.front());
						scopeBoxes.front()->setChecked(true);
					}
					selectedValues.push_back("build-tree");
				}
				m_settings.SetCleanScope(selectedValues.join(';'));
				UpdateRunAvailability();
			};
			for (QCheckBox* scopeBox : scopeBoxes)
			{
				connect(scopeBox, &QCheckBox::toggled, this, updateCleanScopeSetting);
			}
			updateCleanScopeSetting();
			AddMaintenanceEnvironmentStatus(layout, operationId);
			return;
		}

		AddNoOptionsMessage(layout, "No settings");
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
		labelLayout->setContentsMargins(LauncherUi::Option::LabelMargins());
		labelLayout->setSpacing(0);

		QLabel* fieldLabel = CreateFieldLabel(labelCell ? label : label);
		fieldLabel->setAlignment(Qt::AlignLeft | (qobject_cast<QTextEdit*>(control) != nullptr ? Qt::AlignTop : Qt::AlignVCenter));
		fieldLabel->setBuddy(control);
		labelLayout->addWidget(fieldLabel);
		labelCell->setFixedWidth(kFieldLabelWidth + 16);

		QFrame* valueCell = new QFrame(row);
		valueCell->setObjectName("OptionValueCell");
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
		rowLayout->addWidget(valueCell, 1);
		layout.addWidget(row);
		return row;
	}

	QWidget* LauncherMainWindow::AddOptionCheckBox(QVBoxLayout& layout, QCheckBox* checkBox)
	{
		QFrame* row = new QFrame(this);
		row->setObjectName("OptionRow");
		QHBoxLayout* rowLayout = new QHBoxLayout(row);
		rowLayout->setContentsMargins(0, 0, 0, 0);
		rowLayout->setSpacing(0);

		QFrame* labelCell = new QFrame(row);
		labelCell->setObjectName("OptionLabelCell");
		labelCell->setFixedWidth(kFieldLabelWidth + 16);

		QFrame* valueCell = new QFrame(row);
		valueCell->setObjectName("OptionValueCell");
		QHBoxLayout* valueLayout = new QHBoxLayout(valueCell);
		valueLayout->setContentsMargins(LauncherUi::Option::ValueMargins());
		valueLayout->setSpacing(0);
		valueLayout->addWidget(checkBox, 1);

		rowLayout->addWidget(labelCell, 0);
		rowLayout->addWidget(valueCell, 1);
		layout.addWidget(row);
		return row;
	}

	QVBoxLayout* LauncherMainWindow::AddOptionGroup(QVBoxLayout& layout, const QString& title, const QString& detail)
	{
		QFrame* group = new QFrame(this);
		group->setObjectName("OptionGroup");
		QVBoxLayout* groupLayout = new QVBoxLayout(group);
		groupLayout->setContentsMargins(LauncherUi::Option::GroupMargins());
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

	QVBoxLayout* LauncherMainWindow::AddDetailsGroup(QVBoxLayout& layout, const QString& title, const QString& detail, bool expanded)
	{
		QFrame* group = new QFrame(this);
		group->setObjectName("OptionGroup");
		QVBoxLayout* groupLayout = new QVBoxLayout(group);
		groupLayout->setContentsMargins(LauncherUi::Option::GroupMargins());
		groupLayout->setSpacing(LauncherUi::Option::GroupSpacing);

		QToolButton* toggle = new QToolButton(group);
		toggle->setObjectName("DetailsToggleButton");
		toggle->setText(title);
		toggle->setCheckable(true);
		toggle->setChecked(expanded);
		toggle->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
		toggle->setArrowType(expanded ? Qt::DownArrow : Qt::RightArrow);
		RegisterFocusable(toggle);
		groupLayout->addWidget(toggle);

		if (!detail.isEmpty())
		{
			QLabel* detailLabel = new QLabel(detail, group);
			detailLabel->setObjectName("OptionHelpText");
			detailLabel->setWordWrap(true);
			groupLayout->addWidget(detailLabel);
		}

		QFrame* detailsPanel = new QFrame(group);
		detailsPanel->setObjectName("DetailsPanel");
		QVBoxLayout* detailsLayout = new QVBoxLayout(detailsPanel);
		detailsLayout->setContentsMargins(LauncherUi::Option::DetailsMargins());
		detailsLayout->setSpacing(LauncherUi::Option::GroupSpacing);
		detailsPanel->setVisible(expanded);
		groupLayout->addWidget(detailsPanel);

		connect(toggle, &QToolButton::toggled, detailsPanel, [toggle, detailsPanel](bool checked) {
			toggle->setArrowType(checked ? Qt::DownArrow : Qt::RightArrow);
			detailsPanel->setVisible(checked);
		});

		layout.addWidget(group);
		return detailsLayout;
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

