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

#include <QtCore/QSignalBlocker>
#include <QtCore/QStringList>
#include <QtWidgets/QFrame>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSizePolicy>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

#include <cstdint>
#include <filesystem>
#include <system_error>
#include <vector>

namespace SparkleLauncher
{
	static constexpr int kSpaceSmall = LauncherUi::Space::Small;

	QString CompactToolchainDetail(const ToolchainItemStatus& item)
	{
		if (item.State == ToolchainItemState::Found && !item.Path.empty())
		{
			return QString::fromStdString(item.Path.string());
		}
		if (!item.Detail.empty())
		{
			return QString::fromStdString(item.Detail);
		}
		if (!item.Path.empty())
		{
			return QString::fromStdString(item.Path.string());
		}
		return QString();
	}

	void LauncherMainWindow::SelectWorkspaceCompiler(WorkspaceCompiler compiler)
	{
		const QString value = QString::fromStdString(WorkspaceCompilerCommandLineValue(compiler));
		m_settings.SetWorkspaceCompiler(value);
		if (m_workspaceCompilerCombo == nullptr)
		{
			return;
		}

		const int index = m_workspaceCompilerCombo->findData(value);
		if (index >= 0 && index != m_workspaceCompilerCombo->currentIndex())
		{
			const QSignalBlocker blocker(m_workspaceCompilerCombo);
			m_workspaceCompilerCombo->setCurrentIndex(index);
		}
	}

	void LauncherMainWindow::InstallHostTool(const ToolchainItemStatus& item)
	{
		LauncherOperationRequest request =
		    BuildLauncherOperationRequest(m_repositoryRoot, m_contentModel, m_settings, "workspace.install-host-tool");
		request.HostToolId = QString::fromStdString(item.Id);
		StartOperation(std::move(request), "Install " + QString::fromStdString(item.DisplayName));
	}

	QPushButton* LauncherMainWindow::CreateHostToolActionButton(const ToolchainItemStatus& item)
	{
		const std::optional<WorkspaceCompiler> compiler = item.Compiler;
		const bool selected = compiler.has_value() && *compiler == ResolveSelectedWorkspaceCompiler(m_settings);
		if (item.State == ToolchainItemState::Found && (!compiler.has_value() || selected))
		{
			return nullptr;
		}
		if (item.State != ToolchainItemState::Found && !item.CanInstall)
		{
			return nullptr;
		}

		const bool install = item.State != ToolchainItemState::Found;
		const QString label = install ? QStringLiteral("Install") : QStringLiteral("Use");
		const QString displayName = QString::fromStdString(item.DisplayName);
		QPushButton* button = new QPushButton(this);
		ApplyStatusActionButtonPresentation(*button, label, install ? QStringLiteral("warning") : QStringLiteral("neutral"));
		button->setAccessibleName(label + " " + displayName);
		button->setToolTip(
		    install ? "Install " + displayName + ". It becomes selectable after detection confirms the installation."
		            : "Select " + displayName + " for launcher builds.");
		RegisterFocusable(button);
		connect(
		    button,
		    &QPushButton::clicked,
		    this,
		    [this, item, compiler, install]()
		    {
			    if (install)
			    {
				    InstallHostTool(item);
			    }
			    else if (compiler.has_value())
			    {
				    SelectWorkspaceCompiler(*compiler);
			    }
		    });
		return button;
	}

	void LauncherMainWindow::AddBuildEnvironmentStatus(QVBoxLayout& layout, const QString& operationId)
	{
		const BuildWorkspaceOperationRequest request = BuildWorkspacePlanRequest(m_repositoryRoot, m_contentModel, m_settings);

		const QString workspacePlanOperationId =
		    operationId.startsWith("cook.") && operationId != "cook.tools.prepare" ? "cook.tools.prepare" : operationId;
		const BuildWorkspaceOperationPlan plan = PlanBuildWorkspaceOperation(workspacePlanOperationId.toStdString(), request);
		const QString workspaceIdeName = ResolveSelectedWorkspaceIdeName(m_settings);
		const bool isSetupWorkflow = operationId == "workspace.sync-code" || operationId == "workspace.generate-build-files";
		const bool isBuildWorkflow = operationId == "workspace.build-all" || operationId.startsWith("workspace.build")
		    || operationId == "cook.tools.prepare" || operationId == "launcher.build.self";
		const bool isCookWorkflow = operationId.startsWith("cook.") && operationId != "cook.tools.prepare";
		if (isSetupWorkflow)
		{
			const bool isSourceSyncWorkflow = operationId == "workspace.sync-code";
			const QString configurePrerequisiteDetail =
			    !plan.CanRun && !plan.ReadinessMessages.empty() ? QString::fromStdString(plan.ReadinessMessages.back()) : QString();

			QVBoxLayout* machineLayout = AddOptionGroup(layout, "Dependencies", QString());
			const auto addToolchainItems = [this, machineLayout, &plan](bool requiredOnly)
			{
				for (const ToolchainItemStatus& item : plan.Toolchain.Items)
				{
					if (item.Required != requiredOnly)
					{
						continue;
					}
					AddStatusRow(
					    *machineLayout,
					    QString::fromStdString(item.DisplayName),
					    ToolchainStatusText(item.State, item.Required),
					    CompactToolchainDetail(item),
					    ToolchainStatusState(item.State, item.Required),
					    CreateHostToolActionButton(item));
				}
			};
			addToolchainItems(true);
			AddStatusRow(
			    *machineLayout,
			    "Selected IDE",
			    workspaceIdeName,
			    request.PreferredIde == WorkspaceIde::Rider
			        ? (plan.Toolchain.RiderPath.empty() ? "Rider executable was not found."
			                                            : QString::fromStdString(plan.Toolchain.RiderPath.string()))
			        : (plan.Toolchain.VisualStudioIdePath.empty() ? "Visual Studio IDE was not found."
			                                                      : QString::fromStdString(plan.Toolchain.VisualStudioIdePath.string())),
			    request.PreferredIde == WorkspaceIde::Rider ? (plan.Toolchain.RiderPath.empty() ? "warning" : "ok")
			                                                : (plan.Toolchain.VisualStudioIdePath.empty() ? "warning" : "ok"));

			if (!isSourceSyncWorkflow)
			{
				AddStatusRow(
				    *machineLayout,
				    "Build files",
				    plan.Freshness.Current ? "Ready" : "Needs refresh",
				    CombineStatusDetail(QString::fromStdString(plan.Freshness.Summary), BuildFilesRecoveryHint(plan.Freshness)),
				    plan.Freshness.Current ? "ok" : "warning");
			}

			if (!plan.Toolchain.RequiredToolsAvailable)
			{
				AddStatusRow(
				    *machineLayout,
				    "Action needed",
				    "Blocked",
				    RequiredToolProblemSummary(plan.Toolchain),
				    "bad",
				    CreateStatusActionButton("workspace.sync-code", "Review", "Review Sync Code", true));
			}
			else if (!plan.CanRun && !configurePrerequisiteDetail.isEmpty())
			{
				AddStatusRow(
				    *machineLayout,
				    isSourceSyncWorkflow ? "Missing host prerequisite" : "Renderer prerequisites",
				    "Blocked",
				    configurePrerequisiteDetail,
				    "bad",
				    CreateStatusActionButton("workspace.sync-code", "Review", "Review Sync Code", true));
			}
			machineLayout->addSpacing(kSpaceSmall);
			machineLayout->addWidget(CreateSectionLabel("Optional host tools"));
			addToolchainItems(false);
			if (isSourceSyncWorkflow)
			{
				machineLayout->addSpacing(kSpaceSmall);
				machineLayout->addWidget(CreateSectionLabel("Source dependencies"));
				AddSyncDependencies(*machineLayout, false);
				machineLayout->addSpacing(kSpaceSmall);
				machineLayout->addWidget(CreateSectionLabel("Optional source dependencies"));
				AddSyncDependencies(*machineLayout, true);
			}
			return;
		}

		if (isBuildWorkflow)
		{
			QVBoxLayout* buildLayout = AddOptionGroup(layout, "Readiness", QString());
			AddStatusRow(
			    *buildLayout,
			    "Required tools",
			    plan.Toolchain.RequiredToolsAvailable ? "Ready" : "Blocked",
			    plan.Toolchain.RequiredToolsAvailable ? BuildGeneratorSummary(plan.Toolchain) : RequiredToolProblemSummary(plan.Toolchain),
			    plan.Toolchain.RequiredToolsAvailable ? "ok" : "bad",
			    plan.Toolchain.RequiredToolsAvailable
			        ? nullptr
			        : CreateStatusActionButton("workspace.sync-code", "Review", "Review Sync Code", true));
			AddStatusRow(
			    *buildLayout,
			    "Build files",
			    plan.Freshness.Current ? "Ready" : "Needs refresh",
			    CombineStatusDetail(QString::fromStdString(plan.Freshness.Summary), BuildFilesRecoveryHint(plan.Freshness)),
			    plan.Freshness.Current ? "ok" : "warning",
			    plan.Freshness.Current ? nullptr
			                           : CreateStatusActionButton("workspace.generate-build-files", "Generate", "Generate Build Files"));
			return;
		}

		if (isCookWorkflow)
		{
			QVBoxLayout* cookLayout = AddOptionGroup(layout, "Readiness", QString());
			AddStatusRow(
			    *cookLayout,
			    "Required tools",
			    plan.Toolchain.RequiredToolsAvailable ? "Ready" : "Blocked",
			    plan.Toolchain.RequiredToolsAvailable ? BuildGeneratorSummary(plan.Toolchain) : RequiredToolProblemSummary(plan.Toolchain),
			    plan.Toolchain.RequiredToolsAvailable ? "ok" : "bad",
			    plan.Toolchain.RequiredToolsAvailable
			        ? nullptr
			        : CreateStatusActionButton("workspace.sync-code", "Review", "Review Sync Code", true));
			AddStatusRow(
			    *cookLayout,
			    "Build files",
			    plan.Freshness.Current ? "Ready" : "Needs refresh",
			    CombineStatusDetail(QString::fromStdString(plan.Freshness.Summary), BuildFilesRecoveryHint(plan.Freshness)),
			    plan.Freshness.Current ? "ok" : "warning",
			    plan.Freshness.Current ? nullptr
			                           : CreateStatusActionButton("workspace.generate-build-files", "Generate", "Generate Build Files"));
		}
	}

	void LauncherMainWindow::AddMaintenanceEnvironmentStatus(QVBoxLayout& layout, const QString& operationId)
	{
		MaintenanceOperationRequest request;
		request.RepositoryRoot = m_repositoryRoot;
		request.ContentId = m_contentModel.ContentId().toStdString();
		request.EditorProfile = m_settings.EditorProfile().toStdString();
		request.DestructiveActionConfirmed = m_settings.ConfirmClean();

		if (operationId == "workspace.clean")
		{
			QVBoxLayout* maintenanceLayout = AddOptionGroup(layout, "Readiness", QString());
			AddStatusRow(
			    *maintenanceLayout,
			    "Confirmation",
			    m_settings.ConfirmClean() ? "Enabled" : "Required on run",
			    m_settings.ConfirmClean() ? "Clean confirmation is enabled in settings."
			                              : "The launcher will ask for confirmation before destructive clean actions run.",
			    m_settings.ConfirmClean() ? "ok" : "warning");
		}
	}
}
