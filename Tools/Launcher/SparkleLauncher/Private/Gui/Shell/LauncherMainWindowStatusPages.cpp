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
#include "LauncherContentModel.h"
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
	static constexpr const char* kColorStateWarning = LauncherUi::Color::StateWarning;

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

	void LauncherMainWindow::AddBuildEnvironmentStatus(QVBoxLayout& layout, const QString& operationId)
	{
		BuildWorkspaceOperationRequest request;
		request.RepositoryRoot = m_repositoryRoot;
		request.ContentId = m_contentModel.ContentId().toStdString();
		request.EditorProfile = m_settings.EditorProfile().toStdString();
		request.RuntimeProfile = m_settings.RuntimeProfile().toStdString();
		request.PreferredIde = ResolveSelectedWorkspaceIde(m_settings);
		request.ForceConfigure = m_settings.ForceConfigure();

		const QString workspacePlanOperationId =
		    operationId.startsWith("cook.") && operationId != "cook.tools.prepare" ? "cook.tools.prepare" : operationId;
		const BuildWorkspaceOperationPlan plan = PlanBuildWorkspaceOperation(workspacePlanOperationId.toStdString(), request);
		const QString workspaceIdeName = ResolveSelectedWorkspaceIdeName(m_settings);
		const bool isSyncWorkflow = operationId == "workspace.sync-source-tiers" || operationId == "workspace.generate-build-files"
		    || operationId == "workspace.open-ide";
		const bool isBuildWorkflow = operationId == "workspace.build-all" || operationId == "workspace.sync-all"
		    || operationId.startsWith("workspace.build") || operationId == "cook.tools.prepare" || operationId == "launcher.build.self";
		const bool isCookWorkflow = operationId.startsWith("cook.") && operationId != "cook.tools.prepare";
		const std::filesystem::path dependencyCachePath = GetBuildDirectory(m_repositoryRoot) / "_deps";
		const SourceDependencyInventoryStatus dependencyStatus = plan.SourceDependencies;
		const bool dependencyCacheReady = dependencyStatus.AllEnabledDependenciesReady;
		const auto addRelevantDependencyGroups = [this, &dependencyCachePath, &operationId](QVBoxLayout& targetLayout)
		{
			for (const DependencyGroupUiEntry& group : GetDependencyGroups())
			{
				if (!OperationUsesDependencyGroup(operationId, group))
				{
					continue;
				}

				const int readyCount = CountReadyDependencies(group, dependencyCachePath);
				AddStatusRow(
				    targetLayout,
				    group.Label,
				    DependencyGroupStatusText(group, readyCount),
				    FormatDependencyGroupDetail(group, dependencyCachePath, readyCount),
				    DependencyGroupStatusState(group, readyCount),
				    group.Enabled
				        ? CreateActionDependencyActions("workspace.sync-source-tiers", "Sync Code", "deps", "Clean Source Dependency Cache")
				        : CreateDisabledSourceDependencyActions(group));
			}
		};
		if (isSyncWorkflow)
		{
			const bool syncWillRunConfigure = BuildWorkspaceOperationRequiresConfigureStep(plan);
			const bool isGenerateBuildFilesWorkflow = operationId == "workspace.generate-build-files";
			const bool isSourceSyncWorkflow = operationId == "workspace.sync-source-tiers";
			const QString sourceDependencyRepairDetail =
			    syncWillRunConfigure && plan.Freshness.Current && !dependencyStatus.AllEnabledDependenciesReady
			    ? QString("Configure will rerun to repair missing or incomplete enabled source dependencies.")
			    : QString();
			const QString buildFilesDetail = CombineStatusDetail(
			    CombineStatusDetail(QString::fromStdString(plan.Freshness.Summary), BuildFilesRecoveryHint(plan.Freshness)),
			    CombineStatusDetail(
			        sourceDependencyRepairDetail,
			        request.PreferredIde == WorkspaceIde::Rider ? QString("Repository root")
			                                                    : ToDisplayPath(m_repositoryRoot, plan.Freshness.SolutionPath)));
			const QString cacheStatus = dependencyStatus.AllEnabledDependenciesReady ? "Ready"
			    : dependencyStatus.ReadyDependencyCount > 0                          ? "Repair needed"
			                                                                         : "Will be created";
			const QString cacheDetail = dependencyStatus.AllEnabledDependenciesReady ? QString("Repository dependency cache is available.")
			    : dependencyStatus.ReadyDependencyCount > 0 ? QString("Sync Code will repair missing or incomplete repository packages.")
			                                                : QString("Repository dependency cache will be created when Sync Code runs.");
			const QString configurePrerequisiteDetail =
			    !plan.CanRun && !plan.ReadinessMessages.empty() ? QString::fromStdString(plan.ReadinessMessages.back()) : QString();
			int enabledOptionalGroups = 0;
			int readyOptionalGroups = 0;
			for (const DependencyGroupUiEntry& group : GetDependencyGroups())
			{
				if (group.Required || !group.Enabled)
				{
					continue;
				}
				++enabledOptionalGroups;
				if (CountReadyDependencies(group, dependencyCachePath) == static_cast<int>(group.Dependencies.size()))
				{
					++readyOptionalGroups;
				}
			}
			const QString overallStatus = (!plan.Toolchain.RequiredToolsAvailable || !plan.CanRun || !dependencyCacheReady) ? "Needs action"
			    : (syncWillRunConfigure || !plan.Freshness.Current)                                                         ? "Updating"
			                                                                                                                : "Ready";
			const QString overallState = (!plan.Toolchain.RequiredToolsAvailable || !plan.CanRun || !dependencyCacheReady) ? "bad"
			    : (syncWillRunConfigure || !plan.Freshness.Current)                                                        ? "warning"
			                                                                                                               : "ok";
			const QString overallDetail = isSourceSyncWorkflow
			    ? QStringLiteral("Host prerequisites %1 | Repository packages %2/%3 cached")
			          .arg(plan.Toolchain.RequiredToolsAvailable ? "ready" : "blocked")
			          .arg(dependencyStatus.ReadyDependencyCount)
			          .arg(dependencyStatus.EnabledDependencyCount)
			    : QStringLiteral("Machine %1 | Files %2 | Packages %3/%4 cached")
			          .arg(plan.Toolchain.RequiredToolsAvailable ? "ready" : "blocked")
			          .arg(plan.Freshness.Current && !syncWillRunConfigure ? "current" : "updating")
			          .arg(dependencyStatus.ReadyDependencyCount)
			          .arg(dependencyStatus.EnabledDependencyCount);
			if (!isSourceSyncWorkflow)
			{
				QVBoxLayout* summaryLayout = AddOptionGroup(layout, "Workspace readiness overview", QString());
				AddStatusRow(*summaryLayout, "Workspace readiness", overallStatus, overallDetail, overallState);
				AddStatusRow(
				    *summaryLayout,
				    "Build toolchain and IDE",
				    plan.Toolchain.RequiredToolsAvailable ? "Ready" : "Blocked",
				    plan.Toolchain.RequiredToolsAvailable ? BuildGeneratorSummary(plan.Toolchain)
				                                          : RequiredToolProblemSummary(plan.Toolchain),
				    plan.Toolchain.RequiredToolsAvailable ? "ok" : "bad");
				AddStatusRow(
				    *summaryLayout,
				    "Generated workspace files",
				    isGenerateBuildFilesWorkflow ? (plan.Freshness.Current && !syncWillRunConfigure ? "Current" : "Will be refreshed")
				                                 : (plan.Freshness.Current ? "Ready" : "Needs refresh"),
				    buildFilesDetail,
				    plan.Freshness.Current && !syncWillRunConfigure ? "ok" : "warning");
				AddStatusRow(
				    *summaryLayout,
				    "Required repository dependencies",
				    cacheStatus,
				    CombineStatusDetail(cacheDetail, FormatTrackedDependencySummary(dependencyCachePath)),
				    dependencyCacheReady ? "ok" : "warning");
				AddStatusRow(
				    *summaryLayout,
				    "Optional feature dependency groups",
				    enabledOptionalGroups == 0 ? "None enabled" : (readyOptionalGroups == enabledOptionalGroups ? "Ready" : "Partial"),
				    enabledOptionalGroups == 0 ? QString("All optional package groups are off in this workspace.")
				                               : QStringLiteral("%1 of %2 enabled optional package groups are cached.")
				                                     .arg(readyOptionalGroups)
				                                     .arg(enabledOptionalGroups),
				    enabledOptionalGroups == 0 ? "neutral" : (readyOptionalGroups == enabledOptionalGroups ? "ok" : "warning"));
			}

			QVBoxLayout* machineLayout = AddOptionGroup(layout, isSourceSyncWorkflow ? "Dependencies" : "Detected host tools", QString());
			const auto addToolchainItems = [this, machineLayout, &plan, isSourceSyncWorkflow](bool requiredOnly)
			{
				for (const ToolchainItemStatus& item : plan.Toolchain.Items)
				{
					if (item.Required != requiredOnly)
					{
						continue;
					}
					AddStatusRow(
					    *machineLayout,
					    QString::fromStdString(item.DisplayName)
					        + (!isSourceSyncWorkflow && !item.Required ? QStringLiteral(" (optional)") : QString()),
					    ToolchainStatusText(item.State, item.Required),
					    CompactToolchainDetail(item),
					    ToolchainStatusState(item.State, item.Required),
					    nullptr,
					    isSourceSyncWorkflow ? StatusRowPresentation::Inline : StatusRowPresentation::Badge);
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
			        : (plan.Toolchain.VswherePath.empty() ? "Visual Studio discovery is not ready."
			                                              : QString::fromStdString(plan.Freshness.SolutionPath.string())),
			    request.PreferredIde == WorkspaceIde::Rider ? (plan.Toolchain.RiderPath.empty() ? "warning" : "ok")
			                                                : (plan.Toolchain.VswherePath.empty() ? "warning" : "ok"),
			    nullptr,
			    isSourceSyncWorkflow ? StatusRowPresentation::Inline : StatusRowPresentation::Badge);

			if (!plan.Toolchain.RequiredToolsAvailable)
			{
				AddStatusRow(
				    *machineLayout,
				    "Action needed",
				    "Blocked",
				    RequiredToolProblemSummary(plan.Toolchain),
				    "bad",
				    nullptr,
				    isSourceSyncWorkflow ? StatusRowPresentation::Inline : StatusRowPresentation::Badge);
			}
			else if (!plan.CanRun && !configurePrerequisiteDetail.isEmpty())
			{
				AddStatusRow(
				    *machineLayout,
				    isSourceSyncWorkflow ? "Missing host prerequisite" : "Renderer prerequisites",
				    "Blocked",
				    configurePrerequisiteDetail,
				    "bad",
				    nullptr,
				    isSourceSyncWorkflow ? StatusRowPresentation::Inline : StatusRowPresentation::Badge);
			}
			if (operationId == "workspace.sync-source-tiers")
			{
				AddSyncDependencies(*machineLayout, false);
				machineLayout->addSpacing(kSpaceSmall);
				machineLayout->addWidget(CreateSectionLabel("Optional dependencies"));
				addToolchainItems(false);
				AddSyncDependencies(*machineLayout, true);
			}
			else
			{
				addToolchainItems(false);
			}
			return;
		}

		if (isBuildWorkflow)
		{
			const bool buildNeedsAttention = !plan.Toolchain.RequiredToolsAvailable || !plan.Freshness.Current;
			const bool isSyncAllWorkflow = operationId == "workspace.sync-all";
			QVBoxLayout* buildLayout = AddDetailsGroup(
			    layout,
			    isSyncAllWorkflow ? (buildNeedsAttention ? "Sync Dependencies - Needs action" : "Sync Dependencies - Ready")
			                      : (buildNeedsAttention ? "Action Dependencies - Needs action" : "Action Dependencies - Ready"),
			    QString(),
			    false);
			AddStatusRow(
			    *buildLayout,
			    "Required tools",
			    plan.Toolchain.RequiredToolsAvailable ? "Ready" : "Blocked",
			    plan.Toolchain.RequiredToolsAvailable ? BuildGeneratorSummary(plan.Toolchain) : RequiredToolProblemSummary(plan.Toolchain),
			    plan.Toolchain.RequiredToolsAvailable ? "ok" : "bad");
			AddStatusRow(
			    *buildLayout,
			    "Build files",
			    plan.Freshness.Current ? "Ready" : "Needs refresh",
			    CombineStatusDetail(QString::fromStdString(plan.Freshness.Summary), BuildFilesRecoveryHint(plan.Freshness)),
			    plan.Freshness.Current ? "ok" : "warning",
			    CreateActionDependencyActions("workspace.generate-build-files", "Generate Build Files", "build-tree", "Clean Build Files"));
			addRelevantDependencyGroups(*buildLayout);
			return;
		}

		if (isCookWorkflow)
		{
			const bool cookNeedsAttention = !plan.Toolchain.RequiredToolsAvailable || !plan.Freshness.Current;
			QVBoxLayout* cookLayout = AddDetailsGroup(
			    layout,
			    cookNeedsAttention ? "Action Dependencies - Needs action" : "Action Dependencies - Ready",
			    QString(),
			    false);
			AddStatusRow(
			    *cookLayout,
			    "Required tools",
			    plan.Toolchain.RequiredToolsAvailable ? "Ready" : "Blocked",
			    plan.Toolchain.RequiredToolsAvailable ? BuildGeneratorSummary(plan.Toolchain) : RequiredToolProblemSummary(plan.Toolchain),
			    plan.Toolchain.RequiredToolsAvailable ? "ok" : "bad");
			AddStatusRow(
			    *cookLayout,
			    "Build files",
			    plan.Freshness.Current ? "Ready" : "Needs refresh",
			    CombineStatusDetail(QString::fromStdString(plan.Freshness.Summary), BuildFilesRecoveryHint(plan.Freshness)),
			    plan.Freshness.Current ? "ok" : "warning",
			    CreateActionDependencyActions("workspace.generate-build-files", "Generate Build Files", "build-tree", "Clean Build Files"));
			addRelevantDependencyGroups(*cookLayout);
		}
	}

	void LauncherMainWindow::AddLaunchEnvironmentStatus(QVBoxLayout& layout, const QString& operationId)
	{
		LauncherOperationRequest request = BuildLauncherOperationRequest(m_repositoryRoot, m_contentModel, m_settings, operationId);
		BuildWorkspaceOperationRequest workspaceRequest;
		workspaceRequest.RepositoryRoot = m_repositoryRoot;
		workspaceRequest.ContentId = m_contentModel.ContentId().toStdString();
		workspaceRequest.EditorProfile = m_settings.EditorProfile().toStdString();
		workspaceRequest.RuntimeProfile = m_settings.RuntimeProfile().toStdString();
		workspaceRequest.PreferredIde = ResolveSelectedWorkspaceIde(m_settings);
		workspaceRequest.ForceConfigure = m_settings.ForceConfigure();
		LaunchOperationRequest launchRequest;
		launchRequest.RepositoryRoot = request.RepositoryRoot;
		launchRequest.OperationId = operationId.toStdString();
		launchRequest.ContentId = request.ContentId.toStdString();
		launchRequest.EditorProfile = request.EditorProfile.toStdString();
		launchRequest.RuntimeProfile = request.RuntimeProfile.toStdString();
		launchRequest.Target = request.LaunchTarget.toStdString();
		launchRequest.StartupLevel = request.LaunchStartupLevel.toStdString();
		launchRequest.GraphicsBackend = request.LaunchBackend.toStdString();
		launchRequest.VSync = request.LaunchVSync.toStdString();
		launchRequest.PreferHighPerformanceAdapter = request.LaunchHighPerformanceAdapter.toStdString();
		for (const QString& argument : QProcess::splitCommand(request.LaunchCommandLineArguments))
		{
			if (!argument.isEmpty())
			{
				launchRequest.CustomArguments.push_back(argument.toStdString());
			}
		}
		for (const QString& part : request.LaunchCVars.split(QRegularExpression("[,;\\n]"), Qt::SkipEmptyParts))
		{
			const QString trimmed = part.trimmed();
			if (!trimmed.isEmpty())
			{
				launchRequest.CustomCVars.push_back(trimmed.toStdString());
			}
		}

		const LaunchOperationPlan plan = PlanLaunchOperation(operationId.toStdString(), launchRequest);
		const BuildToolchainStatus toolchainStatus = DetectBuildToolchain(workspaceRequest.RepositoryRoot, workspaceRequest.PreferredIde);
		const bool runtimeTarget = launchRequest.Target == "runtime";
		const auto findReadiness = [&plan](const QString& prefix)
		{
			for (const std::string& message : plan.ReadinessMessages)
			{
				const QString text = QString::fromStdString(message);
				if (text.startsWith(prefix, Qt::CaseInsensitive))
				{
					return text;
				}
			}
			return QString();
		};

		const QString executableDetail = findReadiness("Executable ");
		const QString contentDetail = findReadiness("Content working directory ");
		const QString cookedMeshesDetail = findReadiness("Cooked scenes and meshes ");
		const QString cookedTexturesDetail = findReadiness("Cooked textures ");
		const QString cookedShadersDetail = findReadiness("Cooked shaders ");

		const bool launchNeedsAttention = executableDetail.contains("missing", Qt::CaseInsensitive)
		    || contentDetail.contains("missing", Qt::CaseInsensitive) || cookedMeshesDetail.contains("missing", Qt::CaseInsensitive)
		    || cookedTexturesDetail.contains("missing", Qt::CaseInsensitive)
		    || cookedShadersDetail.contains("missing", Qt::CaseInsensitive);
		QVBoxLayout* launchLayout = AddDetailsGroup(
		    layout,
		    launchNeedsAttention ? "Action Dependencies - Needs action" : "Action Dependencies - Ready",
		    QString(),
		    false);
		AddStatusRow(
		    *launchLayout,
		    "Bundled runtime component",
		    "Supported",
		    "Package-root launches use bundled editor/runtime components and cooked assets from dist/; source checkouts use product "
		    "artifacts under artifacts/dev.",
		    "neutral");
		if (launchRequest.GraphicsBackend == "vulkan" && toolchainStatus.VulkanSdkRoot.empty())
		{
			AddStatusRow(
			    *launchLayout,
			    "Graphics backend",
			    "Needs SDK",
			    "Vulkan was selected, but the Vulkan SDK is not available on this machine yet.",
			    "warning");
		}
		AddStatusRow(
		    *launchLayout,
		    runtimeTarget ? "Runtime executable" : "Editor executable",
		    executableDetail.contains("missing", Qt::CaseInsensitive) ? "Missing" : "Ready",
		    executableDetail,
		    executableDetail.contains("missing", Qt::CaseInsensitive) ? "warning" : "ok",
		    CreateActionDependencyActions(
		        runtimeTarget ? "workspace.build.runtime" : "workspace.build.editor",
		        runtimeTarget ? "Build Runtime" : "Build Editor"));
		AddStatusRow(
		    *launchLayout,
		    "Content directory",
		    contentDetail.contains("missing", Qt::CaseInsensitive) ? "Missing" : "Ready",
		    contentDetail,
		    contentDetail.contains("missing", Qt::CaseInsensitive) ? "warning" : "ok");
		AddStatusRow(
		    *launchLayout,
		    "Cooked scene assets",
		    cookedMeshesDetail.contains("missing", Qt::CaseInsensitive) ? "Missing" : "Ready",
		    cookedMeshesDetail,
		    cookedMeshesDetail.contains("missing", Qt::CaseInsensitive) ? "warning" : "ok",
		    CreateActionDependencyActions("cook.assets", "Cook Scenes And Meshes"));
		AddStatusRow(
		    *launchLayout,
		    "Cooked textures",
		    cookedTexturesDetail.contains("missing", Qt::CaseInsensitive) ? "Missing" : "Ready",
		    cookedTexturesDetail,
		    cookedTexturesDetail.contains("missing", Qt::CaseInsensitive) ? "warning" : "ok",
		    CreateActionDependencyActions("cook.textures", "Cook Textures"));
		AddStatusRow(
		    *launchLayout,
		    "Cooked shaders",
		    cookedShadersDetail.contains("missing", Qt::CaseInsensitive) ? "Missing" : "Ready",
		    cookedShadersDetail,
		    cookedShadersDetail.contains("missing", Qt::CaseInsensitive) ? "warning" : "ok",
		    CreateActionDependencyActions("cook.shaders", "Cook Shaders"));
	}

	void LauncherMainWindow::AddLaunchTargetOptions(QVBoxLayout& layout, const QString& title, const QString& detail)
	{
		QVBoxLayout* targetLayout = AddOptionGroup(layout, title, detail);
		AddOptionField(
		    *targetLayout,
		    "Target",
		    CreateValueCombo(
		        {{"Editor", "editor"}, {"Runtime", "runtime"}},
		        m_settings.LaunchTarget(),
		        &LauncherSettings::SetLaunchTarget));
	}

	void LauncherMainWindow::AddLaunchApplicationOptions(QVBoxLayout& layout)
	{
		QVBoxLayout* appOptionsLayout = AddOptionGroup(layout, "Options", QString());
		AddOptionField(
		    *appOptionsLayout,
		    "Graphics backend",
		    CreateValueCombo({{"D3D12", ""}, {"Vulkan", "vulkan"}}, m_settings.LaunchBackend(), &LauncherSettings::SetLaunchBackend));
		AddOptionField(
		    *appOptionsLayout,
		    "VSync",
		    CreateValueCombo({{"On", ""}, {"Off", "false"}}, m_settings.LaunchVSync(), &LauncherSettings::SetLaunchVSync));
		AddOptionField(
		    *appOptionsLayout,
		    "GPU preference",
		    CreateValueCombo(
		        {{"High performance", ""}, {"System default", "false"}},
		        m_settings.LaunchHighPerformanceAdapter(),
		        &LauncherSettings::SetLaunchHighPerformanceAdapter));
		AddOptionField(
		    *appOptionsLayout,
		    "Arguments",
		    CreateBoundLineEdit(
		        m_settings.LaunchCommandLineArguments(),
		        "--flag value \"quoted value\"",
		        "Extra command-line arguments appended after launcher-managed options.",
		        &LauncherSettings::SetLaunchCommandLineArguments));
		AddOptionField(
		    *appOptionsLayout,
		    "CVars",
		    CreateBoundTextEdit(
		        m_settings.LaunchCVars(),
		        "r.SomeCVar=1\nr.OtherCVar=false",
		        "One CVar assignment per line, comma, or semicolon. Each entry is passed as --cvar name=value.",
		        &LauncherSettings::SetLaunchCVars));
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
			QVBoxLayout* maintenanceLayout = AddDetailsGroup(
			    layout,
			    m_settings.ConfirmClean() ? "Action Dependencies - Ready" : "Action Dependencies - Confirmation required",
			    QString(),
			    false);
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
