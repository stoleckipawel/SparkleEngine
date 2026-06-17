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
	static constexpr const char* kColorStateWarning = LauncherUi::Color::StateWarning;

	namespace
	{
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
	}

	void LauncherMainWindow::AddBuildEnvironmentStatus(QVBoxLayout& layout, const QString& operationId)
	{
		BuildWorkspaceOperationRequest request;
		request.RepositoryRoot = m_repositoryRoot;
		request.ProjectId = m_projectModel.ActiveProjectId().toStdString();
		request.EditorProfile = m_settings.EditorProfile().toStdString();
		request.RuntimeProfile = m_settings.RuntimeProfile().toStdString();
		request.PreferredIde = ResolveSelectedWorkspaceIde(m_settings);
		request.ForceConfigure = m_settings.ForceConfigure();

		const QString workspacePlanOperationId =
		    operationId.startsWith("cook.") && operationId != "cook.tools.prepare" ? "cook.tools.prepare" : operationId;
		const BuildWorkspaceOperationPlan plan = PlanBuildWorkspaceOperation(workspacePlanOperationId.toStdString(), request);
		const QString workspaceIdeName = ResolveSelectedWorkspaceIdeName(m_settings);
		const bool isToolchainCheck = operationId == "toolchain.check";
		const bool isSyncWorkflow = operationId == "workspace.sync-source-tiers" || operationId == "workspace.generate-build-files" || operationId == "workspace.open-ide";
		const bool isBuildWorkflow = operationId == "workspace.build-all" || operationId.startsWith("project.build") || operationId == "cook.tools.prepare" || operationId == "launcher.build.self";
		const bool isCookWorkflow = operationId.startsWith("cook.") && operationId != "cook.tools.prepare";
		const std::filesystem::path dependencyCachePath = GetBuildDirectory(m_repositoryRoot) / "_deps";
		const SourceDependencyInventoryStatus dependencyStatus = plan.SourceDependencies;
		const bool dependencyCacheReady = dependencyStatus.AllEnabledDependenciesReady;
		const auto addRelevantDependencyGroups = [this, &dependencyCachePath, &operationId](QVBoxLayout& targetLayout) {
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
				    group.Enabled ? CreateActionDependencyActions("workspace.sync-source-tiers", "Prepare Workspace", "deps", "Clean Source Dependency Cache") :
				                    CreateDisabledSourceTierActions(group));
			}
		};
		const auto addHostDependencyStatus = [this, &plan, &request, &workspaceIdeName](QVBoxLayout& targetLayout, const QString& detailText) {
			QVBoxLayout* hostLayout = AddDetailsGroup(
			    targetLayout,
			    "Host Rebuild Dependency Details",
			    detailText,
			    !plan.Toolchain.RequiredToolsAvailable);
			AddStatusRow(
			    *hostLayout,
			    "Dependency set",
			    plan.Toolchain.RequiredToolsAvailable ? "Ready" : "Action needed",
			    BuildGeneratorSummary(plan.Toolchain),
			    plan.Toolchain.RequiredToolsAvailable ? "ok" : "bad",
			    CreateActionDependencyActions("toolchain.check", "Verify Host Environment"));
			if (plan.Toolchain.RequiredToolsAvailable)
			{
				AddStatusRow(
				    *hostLayout,
				    "Tool details",
				    "Available in host audit",
				    "Open Verify Host Environment for the full installed-tool inventory.",
				    "neutral",
				    CreateActionDependencyActions("toolchain.check", "Verify Host Environment", QString(), QString(), true));
				return;
			}
			for (const ToolchainItemStatus& item : plan.Toolchain.Items)
			{
				QString detail = QString::fromStdString(item.Detail);
				const QString path = FormatStatusPath(item.Path);
				if (!path.isEmpty())
				{
					detail = CombineStatusDetail(detail, path);
				}
				AddStatusRow(
				    *hostLayout,
				    QString::fromStdString(item.DisplayName) + (item.Required ? "" : " (optional)"),
				    ToolchainStatusText(item.State, item.Required),
				    detail,
				    ToolchainStatusState(item.State, item.Required));
			}
			AddStatusRow(
			    *hostLayout,
			    "Selected IDE",
			    workspaceIdeName,
			    request.PreferredIde == WorkspaceIde::Rider ? (plan.Toolchain.RiderPath.empty() ? "Rider executable was not found." : QString::fromStdString(plan.Toolchain.RiderPath.string())) :
			                                                 (plan.Toolchain.VswherePath.empty() ? "Visual Studio discovery is not ready." : QString::fromStdString(plan.Freshness.SolutionPath.string())),
			    request.PreferredIde == WorkspaceIde::Rider ? (plan.Toolchain.RiderPath.empty() ? "warning" : "ok") : (plan.Toolchain.VswherePath.empty() ? "warning" : "ok"));
		};
		if (isToolchainCheck)
		{
			QVBoxLayout* hostDetailsLayout = AddOptionGroup(
			    layout,
			    "Machine support",
			    "Installed tools and detected paths used for local builds, IDE integration, and optional renderer features.");
			for (const ToolchainItemStatus& item : plan.Toolchain.Items)
			{
				QString detail = QString::fromStdString(item.Detail);
				const QString path = FormatStatusPath(item.Path);
				if (!path.isEmpty())
				{
					detail = CombineStatusDetail(detail, path);
				}
				AddStatusRow(
				    *hostDetailsLayout,
				    QString::fromStdString(item.DisplayName) + (item.Required ? "" : " (optional)"),
				    ToolchainStatusText(item.State, item.Required),
				    detail,
				    ToolchainStatusState(item.State, item.Required));
			}
			AddStatusRow(
			    *hostDetailsLayout,
			    "Selected IDE",
			    workspaceIdeName,
			    request.PreferredIde == WorkspaceIde::Rider ? (plan.Toolchain.RiderPath.empty() ? "Rider executable was not found." : QString::fromStdString(plan.Toolchain.RiderPath.string())) :
			                                                 (plan.Toolchain.VswherePath.empty() ? "Visual Studio discovery is not ready." : QString::fromStdString(plan.Freshness.SolutionPath.string())),
			    request.PreferredIde == WorkspaceIde::Rider ? (plan.Toolchain.RiderPath.empty() ? "warning" : "ok") : (plan.Toolchain.VswherePath.empty() ? "warning" : "ok"));
			QVBoxLayout* toolchainLayout = AddOptionGroup(
			    layout,
			    "Readiness summary",
			    "Use this audit to confirm the machine can generate workspace files, open the IDE, and rebuild locally.");
			AddStatusRow(*toolchainLayout, "Dependency set", plan.Toolchain.RequiredToolsAvailable ? "Ready" : "Action needed", BuildGeneratorSummary(plan.Toolchain), plan.Toolchain.RequiredToolsAvailable ? "ok" : "bad");
			AddStatusRow(
			    *toolchainLayout,
			    "Selected IDE",
			    workspaceIdeName,
			    request.PreferredIde == WorkspaceIde::Rider ? (plan.Toolchain.RiderPath.empty() ? "Rider executable was not found." : "Rider executable is available.") :
			                                                 (plan.Toolchain.VswherePath.empty() ? "Visual Studio discovery is not ready." : "Visual Studio workspace discovery is available."),
			    request.PreferredIde == WorkspaceIde::Rider ? (plan.Toolchain.RiderPath.empty() ? "warning" : "ok") : (plan.Toolchain.VswherePath.empty() ? "warning" : "ok"));
			return;
		}

		if (isSyncWorkflow)
		{
			const bool syncWillRunConfigure = BuildWorkspaceOperationRequiresConfigureStep(plan);
			const bool isGenerateBuildFilesWorkflow = operationId == "workspace.generate-build-files";
			const bool isSourceSyncWorkflow = operationId == "workspace.sync-source-tiers";
			const QString sourceDependencyRepairDetail = syncWillRunConfigure && plan.Freshness.Current && !dependencyStatus.AllEnabledDependenciesReady ?
			                                                 QString("Configure will rerun to repair missing or incomplete enabled source dependencies.") :
			                                                 QString();
			const QString buildFilesDetail = CombineStatusDetail(
			    CombineStatusDetail(QString::fromStdString(plan.Freshness.Summary), BuildFilesRecoveryHint(plan.Freshness)),
			    CombineStatusDetail(
			        sourceDependencyRepairDetail,
			        request.PreferredIde == WorkspaceIde::Rider ? QString("Repository root") : ToDisplayPath(m_repositoryRoot, plan.Freshness.SolutionPath)));
			const QString cacheStatus = dependencyStatus.AllEnabledDependenciesReady ? "Ready" :
			                           dependencyStatus.ReadyDependencyCount > 0 ? "Repair needed" :
			                                                                  "Will be created";
			const QString cacheDetail = dependencyStatus.AllEnabledDependenciesReady ?
			                               QString("Repository dependency cache is available.") :
			                           dependencyStatus.ReadyDependencyCount > 0 ?
			                               QString("Prepare Workspace will repair missing or incomplete repository packages.") :
			                               QString("Repository dependency cache will be created when Prepare Workspace runs.");
			const QString configurePrerequisiteDetail = !plan.CanRun && !plan.ReadinessMessages.empty() ?
			                                               QString::fromStdString(plan.ReadinessMessages.back()) :
			                                               QString();
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
			const QString overallStatus = (!plan.Toolchain.RequiredToolsAvailable || !plan.CanRun || !dependencyCacheReady) ? "Needs action" :
			                             (syncWillRunConfigure || !plan.Freshness.Current) ? "Updating" :
			                                                                            "Ready";
			const QString overallState = (!plan.Toolchain.RequiredToolsAvailable || !plan.CanRun || !dependencyCacheReady) ? "bad" :
			                            (syncWillRunConfigure || !plan.Freshness.Current) ? "warning" :
			                                                                             "ok";
			const QString overallDetail = isSourceSyncWorkflow ?
			                                 QStringLiteral("Host prerequisites %1 | Repository packages %2/%3 cached")
			                                     .arg(plan.Toolchain.RequiredToolsAvailable ? "ready" : "blocked")
			                                     .arg(dependencyStatus.ReadyDependencyCount)
			                                     .arg(dependencyStatus.EnabledDependencyCount) :
			                                 QStringLiteral("Machine %1 | Files %2 | Packages %3/%4 cached")
			                                     .arg(plan.Toolchain.RequiredToolsAvailable ? "ready" : "blocked")
			                                     .arg(plan.Freshness.Current && !syncWillRunConfigure ? "current" : "updating")
			                                     .arg(dependencyStatus.ReadyDependencyCount)
			                                     .arg(dependencyStatus.EnabledDependencyCount);
			QVBoxLayout* summaryLayout = AddOptionGroup(
			    layout,
			    isSourceSyncWorkflow ? "Sync readiness overview" : "Workspace readiness overview",
			    QString());
			AddStatusRow(
			    *summaryLayout,
			    isSourceSyncWorkflow ? "Sync readiness" : "Workspace readiness",
			    overallStatus,
			    overallDetail,
			    overallState);
			AddStatusRow(
			    *summaryLayout,
			    isSourceSyncWorkflow ? "Host-installed prerequisites" : "Build toolchain and IDE",
			    plan.Toolchain.RequiredToolsAvailable ? "Ready" : "Blocked",
			    plan.Toolchain.RequiredToolsAvailable ? BuildGeneratorSummary(plan.Toolchain) : RequiredToolProblemSummary(plan.Toolchain),
			    plan.Toolchain.RequiredToolsAvailable ? "ok" : "bad");
			if (!isSourceSyncWorkflow)
			{
				AddStatusRow(
				    *summaryLayout,
				    "Generated workspace files",
				    isGenerateBuildFilesWorkflow ? (plan.Freshness.Current && !syncWillRunConfigure ? "Current" : "Will be refreshed") :
				                           (plan.Freshness.Current ? "Ready" : "Needs refresh"),
				    buildFilesDetail,
				    plan.Freshness.Current && !syncWillRunConfigure ? "ok" : "warning");
			}
			AddStatusRow(
			    *summaryLayout,
			    isSourceSyncWorkflow ? "Synced repository packages" : "Required repository dependencies",
			    cacheStatus,
			    CombineStatusDetail(cacheDetail, FormatTrackedDependencySummary(dependencyCachePath)),
			    dependencyCacheReady ? "ok" : "warning");
			AddStatusRow(
			    *summaryLayout,
			    isSourceSyncWorkflow ? "Optional repository feature packs" : "Optional feature dependency groups",
			    enabledOptionalGroups == 0 ? "None enabled" : (readyOptionalGroups == enabledOptionalGroups ? "Ready" : "Partial"),
			    enabledOptionalGroups == 0 ?
			        QString("All optional package groups are off in this workspace.") :
			        QStringLiteral("%1 of %2 enabled optional package groups are cached.").arg(readyOptionalGroups).arg(enabledOptionalGroups),
			    enabledOptionalGroups == 0 ? "neutral" : (readyOptionalGroups == enabledOptionalGroups ? "ok" : "warning"));

			QVBoxLayout* machineLayout = AddOptionGroup(
			    layout,
			    isSourceSyncWorkflow ? "Installed host tools" : "Detected host tools",
			    QString());
			for (const ToolchainItemStatus& item : plan.Toolchain.Items)
			{
				AddStatusRow(
				    *machineLayout,
				    QString::fromStdString(item.DisplayName) + (item.Required ? "" : " (optional)"),
				    ToolchainStatusText(item.State, item.Required),
				    CompactToolchainDetail(item),
				    ToolchainStatusState(item.State, item.Required));
			}
			AddStatusRow(
			    *machineLayout,
			    "Selected IDE",
			    workspaceIdeName,
			    request.PreferredIde == WorkspaceIde::Rider ?
			        (plan.Toolchain.RiderPath.empty() ? "Rider executable was not found." : QString::fromStdString(plan.Toolchain.RiderPath.string())) :
			        (plan.Toolchain.VswherePath.empty() ? "Visual Studio discovery is not ready." : QString::fromStdString(plan.Freshness.SolutionPath.string())),
			    request.PreferredIde == WorkspaceIde::Rider ? (plan.Toolchain.RiderPath.empty() ? "warning" : "ok") : (plan.Toolchain.VswherePath.empty() ? "warning" : "ok"));

			if (!plan.Toolchain.RequiredToolsAvailable)
			{
				AddStatusRow(
				    *machineLayout,
				    "Action needed",
				    "Blocked",
				    RequiredToolProblemSummary(plan.Toolchain),
				    "bad");
			}
			else if (!plan.CanRun && !configurePrerequisiteDetail.isEmpty())
			{
				AddStatusRow(
				    *machineLayout,
				    isSourceSyncWorkflow ? "Missing host prerequisite" : "Renderer prerequisites",
				    "Blocked",
				    configurePrerequisiteDetail,
				    "bad");
			}
			if (operationId == "workspace.sync-source-tiers")
			{
				AddSyncDependencyBundles(layout, true);
			}
			return;
		}

		if (isBuildWorkflow)
		{
			const bool buildNeedsAttention = !plan.Toolchain.RequiredToolsAvailable || !plan.Freshness.Current;
			addHostDependencyStatus(
			    layout,
			    operationId == "launcher.build.self" ?
			        "Even if this launcher came from a ready-to-run package, rebuilding it locally still requires Visual Studio/MSVC, a Qt 6 MSVC kit, CMake, Git, and the Windows SDK." :
			        "Prebuilt package binaries do not remove the local host dependencies needed to rebuild this workspace.");
			QVBoxLayout* buildLayout = AddDetailsGroup(
			    layout,
			    buildNeedsAttention ? "Action Dependencies - Needs action" : "Action Dependencies - Ready",
			    "Requirements this build workflow depends on before a local rebuild can run successfully.",
			    false);
			AddStatusRow(
			    *buildLayout,
			    "Required tools",
			    plan.Toolchain.RequiredToolsAvailable ? "Ready" : "Blocked",
			    plan.Toolchain.RequiredToolsAvailable ? BuildGeneratorSummary(plan.Toolchain) : RequiredToolProblemSummary(plan.Toolchain),
			    plan.Toolchain.RequiredToolsAvailable ? "ok" : "bad",
			    CreateActionDependencyActions("toolchain.check", "Verify Host Environment"));
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
			addHostDependencyStatus(
			    layout,
			    "Cook workflows may ship ready-to-use outputs, but rebuilding or recooking them locally still requires the same host build dependencies and any enabled optional dependency groups.");
			QVBoxLayout* cookLayout = AddDetailsGroup(
			    layout,
			    cookNeedsAttention ? "Action Dependencies - Needs action" : "Action Dependencies - Ready",
			    "Requirements this cook workflow depends on before local recook operations can run successfully.",
			    false);
			AddStatusRow(
			    *cookLayout,
			    "Required tools",
			    plan.Toolchain.RequiredToolsAvailable ? "Ready" : "Blocked",
			    plan.Toolchain.RequiredToolsAvailable ? BuildGeneratorSummary(plan.Toolchain) : RequiredToolProblemSummary(plan.Toolchain),
			    plan.Toolchain.RequiredToolsAvailable ? "ok" : "bad",
			    CreateActionDependencyActions("toolchain.check", "Verify Host Environment"));
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
		LauncherOperationRequest request = BuildLauncherOperationRequest(m_repositoryRoot, m_projectModel, m_settings, operationId);
		LaunchOperationRequest launchRequest;
		launchRequest.RepositoryRoot = request.RepositoryRoot;
		launchRequest.OperationId = operationId.toStdString();
		launchRequest.ProjectId = request.ProjectId.toStdString();
		launchRequest.EditorProfile = request.EditorProfile.toStdString();
		launchRequest.RuntimeProfile = request.RuntimeProfile.toStdString();
		launchRequest.Target = request.LaunchTarget.toStdString();
		launchRequest.StartupLevel = request.LaunchStartupLevel.toStdString();
		launchRequest.EnableSmokeTest = request.LaunchSmokeTest;
		launchRequest.GraphicsBackend = request.LaunchBackend.toStdString();
		launchRequest.VSync = request.LaunchVSync.toStdString();
		launchRequest.PreferHighPerformanceAdapter = request.LaunchHighPerformanceAdapter.toStdString();
		launchRequest.MeshAutoBatching = request.LaunchMeshAutoBatching.toStdString();
		launchRequest.PreferPartitionedTlas = request.LaunchPreferPartitionedTlas.toStdString();
		launchRequest.PtlasOperationWriterPath = request.LaunchPtlasOperationWriterPath.toStdString();
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
		launchRequest.SmokeBackend = request.SmokeBackend.toStdString();
		launchRequest.SmokeFrameLimit = request.SmokeFrameLimit.toStdString();
		launchRequest.SmokeViewMode = request.SmokeViewMode.toStdString();
		launchRequest.SmokeCapturePath = request.SmokeCapturePath.toStdString();
		launchRequest.SmokeTrace = request.SmokeTrace;
		launchRequest.SmokeSkipLevelSwitching = request.SmokeSkipLevelSwitching;
		launchRequest.SmokeRunRayTracingParity = request.SmokeRunRayTracingParity;
		launchRequest.SmokeRunPtlasBenchmark = request.SmokeRunPtlasBenchmark;
		launchRequest.SmokeRunDiagnosticCaptures = request.SmokeRunDiagnosticCaptures;

		const LaunchOperationPlan plan = PlanLaunchOperation(operationId.toStdString(), launchRequest);
		const bool runtimeTarget = launchRequest.Target == "runtime";
		const auto findReadiness = [&plan](const QString& prefix) {
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
		const QString projectDetail = findReadiness("Project working directory ");
		const QString cookedMeshesDetail = findReadiness("Cooked scene assets ");
		const QString cookedTexturesDetail = findReadiness("Cooked textures ");
		const QString cookedShadersDetail = findReadiness("Cooked shaders ");

		const bool launchNeedsAttention =
		    executableDetail.contains("missing", Qt::CaseInsensitive) ||
		    projectDetail.contains("missing", Qt::CaseInsensitive) ||
		    cookedMeshesDetail.contains("missing", Qt::CaseInsensitive) ||
		    cookedTexturesDetail.contains("missing", Qt::CaseInsensitive) ||
		    cookedShadersDetail.contains("missing", Qt::CaseInsensitive);
		QVBoxLayout* launchLayout = AddDetailsGroup(
		    layout,
		    launchNeedsAttention ? "Action Dependencies - Needs action" : "Action Dependencies - Ready",
		    "Launch workflows use bundled runtime components when packages provide them, then local rebuild outputs when developing from source.",
		    false);
		AddStatusRow(
		    *launchLayout,
		    "Bundled runtime component",
		    "Supported",
		    "Package-root launches use bundled editor/runtime components and cooked assets from dist/; source checkouts use product artifacts under artifacts/dev.",
		    "neutral");
		AddStatusRow(
		    *launchLayout,
		    runtimeTarget ? "Runtime executable" : "Editor executable",
		    executableDetail.contains("missing", Qt::CaseInsensitive) ? "Missing" : "Ready",
		    executableDetail,
		    executableDetail.contains("missing", Qt::CaseInsensitive) ? "warning" : "ok",
		    CreateActionDependencyActions(runtimeTarget ? "project.build.runtime" : "project.build.editor", runtimeTarget ? "Build Runtime" : "Build Editor"));
		AddStatusRow(
		    *launchLayout,
		    "Project directory",
		    projectDetail.contains("missing", Qt::CaseInsensitive) ? "Missing" : "Ready",
		    projectDetail,
		    projectDetail.contains("missing", Qt::CaseInsensitive) ? "warning" : "ok");
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

	void LauncherMainWindow::AddLaunchTargetOptions(QVBoxLayout& layout, const QString& title, const QString& detail, bool includeStartupLevel)
	{
		QVBoxLayout* targetLayout = AddOptionGroup(layout, title, detail);
		AddOptionField(
		    *targetLayout,
		    "Target",
		    CreateValueCombo(
		        {{"Editor", "editor"}, {"Runtime", "runtime"}},
		        m_settings.LaunchTarget(),
		        &LauncherSettings::SetLaunchTarget));
		if (includeStartupLevel)
		{
			AddOptionField(
			    *targetLayout,
			    "Startup level",
			    CreateValueCombo(
			        {{"Sponza", "Sponza"},
			            {"A Beautiful Game", "ABeautifulGame"},
			            {"Damaged Helmet", "DamagedHelmet"},
			            {"Cesium Man", "CesiumMan"},
			            {"Diffuse Transmission Plant", "DiffuseTransmissionPlant"},
			            {"Empty", "Empty"}},
			        m_settings.LaunchStartupLevel(),
			        &LauncherSettings::SetLaunchStartupLevel));
		}
	}

	void LauncherMainWindow::AddLaunchApplicationOptions(QVBoxLayout& layout)
	{
		QVBoxLayout* appOptionsLayout = AddOptionGroup(layout, "Options", "Arguments and runtime CVars passed to the selected process.");
		AddOptionField(
		    *appOptionsLayout,
		    "Graphics backend",
		    CreateValueCombo(
		        {{"D3D12", ""}, {"Vulkan", "vulkan"}},
		        m_settings.LaunchBackend(),
		        &LauncherSettings::SetLaunchBackend));
		AddOptionField(
		    *appOptionsLayout,
		    "VSync",
		    CreateValueCombo(
		        {{"On", ""}, {"Off", "false"}},
		        m_settings.LaunchVSync(),
		        &LauncherSettings::SetLaunchVSync));
		AddOptionField(
		    *appOptionsLayout,
		    "GPU preference",
		    CreateValueCombo(
		        {{"High performance", ""}, {"System default", "false"}},
		        m_settings.LaunchHighPerformanceAdapter(),
		        &LauncherSettings::SetLaunchHighPerformanceAdapter));
		AddOptionField(
		    *appOptionsLayout,
		    "Ray tracing TLAS",
		    CreateValueCombo(
		        {{"Partitioned TLAS", "true"}, {"Classic TLAS", "false"}},
		        m_settings.LaunchPreferPartitionedTlas(),
		        &LauncherSettings::SetLaunchPreferPartitionedTlas));
		AddOptionField(
		    *appOptionsLayout,
		    "PTLAS update path",
		    CreateValueCombo(
		        {
		            {"CPU pack", "1"},
		            {"GPU dirty + CPU native pack", "2"},
		            {"Full GPU native pack", "3"},
		        },
		        m_settings.LaunchPtlasOperationWriterPath(),
		        &LauncherSettings::SetLaunchPtlasOperationWriterPath));
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

	void LauncherMainWindow::AddSmokeValidationOptions(QVBoxLayout& layout)
	{
		QVBoxLayout* smokeOptionsLayout = AddOptionGroup(
		    layout,
		    "Smoke Tests",
		    "Shared smoke controls first, PTLAS-specific suites last.");

		auto addCompactCheckBoxRow = [this](QVBoxLayout& sectionLayout, QCheckBox* checkBox, int maxWidth = 520) {
			QFrame* row = new QFrame(this);
			row->setObjectName("OptionRow");
			row->setMaximumWidth(maxWidth);
			QHBoxLayout* rowLayout = new QHBoxLayout(row);
			rowLayout->setContentsMargins(0, 0, 0, 0);
			rowLayout->setSpacing(0);

			QFrame* valueCell = new QFrame(row);
			valueCell->setObjectName("OptionValueCell");
			QHBoxLayout* valueLayout = new QHBoxLayout(valueCell);
			valueLayout->setContentsMargins(0, 0, 0, 0);
			valueLayout->setSpacing(0);
			valueLayout->addWidget(checkBox, 1);

			rowLayout->addWidget(valueCell, 1);
			sectionLayout.addWidget(row, 0, Qt::AlignLeft);
		};

		auto addCompactFieldRow = [this](QVBoxLayout& sectionLayout, const QString& label, QWidget* control, int maxWidth = 820) {
			QFrame* row = new QFrame(this);
			row->setObjectName("OptionRow");
			row->setMaximumWidth(maxWidth);
			QHBoxLayout* rowLayout = new QHBoxLayout(row);
			rowLayout->setContentsMargins(0, 0, 0, 0);
			rowLayout->setSpacing(0);

			QFrame* labelCell = new QFrame(row);
			labelCell->setObjectName("OptionLabelCell");
			labelCell->setFixedWidth(108);
			QHBoxLayout* labelLayout = new QHBoxLayout(labelCell);
			labelLayout->setContentsMargins(8, 0, 8, 0);
			labelLayout->setSpacing(0);

			QLabel* fieldLabel = CreateFieldLabel(label);
			fieldLabel->setBuddy(control);
			labelLayout->addWidget(fieldLabel);

			QFrame* valueCell = new QFrame(row);
			valueCell->setObjectName("OptionValueCell");
			QHBoxLayout* valueLayout = new QHBoxLayout(valueCell);
			valueLayout->setContentsMargins(0, 0, 0, 0);
			valueLayout->setSpacing(0);
			valueLayout->addWidget(control, 1);

			rowLayout->addWidget(labelCell, 0);
			rowLayout->addWidget(valueCell, 1);
			sectionLayout.addWidget(row, 0, Qt::AlignLeft);
		};

		addCompactFieldRow(
		    *smokeOptionsLayout,
		    "Frame limit",
		    CreateValueCombo(
		        {{"120 frames", ""}, {"60 frames", "60"}, {"300 frames", "300"}, {"600 frames", "600"}},
		        m_settings.SmokeFrameLimit(),
		        &LauncherSettings::SetSmokeFrameLimit));
		addCompactFieldRow(
		    *smokeOptionsLayout,
		    "View mode",
		    CreateValueCombo(
		        {
		            {"Default", ""},
		            {"Lit", "0"},
		            {"Wireframe", "1"},
		            {"GBuffer normal", "3"},
		            {"Direct diffuse", "10"},
		            {"Indirect diffuse", "13"},
		            {"Instance groups", "16"},
		        },
		        m_settings.SmokeViewMode(),
		        &LauncherSettings::SetSmokeViewMode));
		addCompactFieldRow(
		    *smokeOptionsLayout,
		    "Capture path",
		    CreateBoundLineEdit(
		        m_settings.SmokeCapturePath(),
		        "logs/smoke/scene-color.bmp",
		        "Optional base scene-color capture path. Suite-specific diagnostics still write their own artifacts.",
		        &LauncherSettings::SetSmokeCapturePath));
		addCompactCheckBoxRow(
		    *smokeOptionsLayout,
		    CreateBoundCheckBox(
		        "Capture trace",
		        "Write smoke trace output to help diagnose failures or correlate capture artifacts with runtime events.",
		        m_settings.SmokeTrace(),
		        &LauncherSettings::SetSmokeTrace),
		    420);
		addCompactCheckBoxRow(
		    *smokeOptionsLayout,
		    CreateBoundCheckBox(
		        "Skip level switching",
		        "Do not switch levels during smoke. Matrix-oriented suites force this behavior when deterministic comparisons require a fixed scene.",
		        m_settings.SmokeSkipLevelSwitching(),
		        &LauncherSettings::SetSmokeSkipLevelSwitching),
		    420);

		QLabel* ptlasLabel = CreateSectionLabel("PTLAS Suites");
		smokeOptionsLayout->addWidget(ptlasLabel);

		addCompactCheckBoxRow(
		    *smokeOptionsLayout,
		    CreateBoundCheckBox(
		        "Backend/PTLAS parity checks",
		        "Validates deterministic D3D12 and Vulkan parity and confirms PTLAS stays aligned with the fallback path.",
		        m_settings.SmokeRunRayTracingParity(),
		        &LauncherSettings::SetSmokeRunRayTracingParity));
		addCompactCheckBoxRow(
		    *smokeOptionsLayout,
		    CreateBoundCheckBox(
		        "PTLAS benchmark timings",
		        "Runs PTLAS benchmark scenarios and emits timing-oriented diagnostics for performance review.",
		        m_settings.SmokeRunPtlasBenchmark(),
		        &LauncherSettings::SetSmokeRunPtlasBenchmark));
		addCompactCheckBoxRow(
		    *smokeOptionsLayout,
		    CreateBoundCheckBox(
		        "PTLAS diagnostic captures",
		        "Generates capture-oriented PTLAS diagnostic views and writes a reusable artifact bundle for inspection.",
		        m_settings.SmokeRunDiagnosticCaptures(),
		        &LauncherSettings::SetSmokeRunDiagnosticCaptures));
	}

	void LauncherMainWindow::AddMaintenanceEnvironmentStatus(QVBoxLayout& layout, const QString& operationId)
	{
		MaintenanceOperationRequest request;
		request.RepositoryRoot = m_repositoryRoot;
		request.ProjectId = m_projectModel.ActiveProjectId().toStdString();
		request.EditorProfile = m_settings.EditorProfile().toStdString();
		request.RequestedFormatMode = m_settings.FormatMode() == "check" ? FormatMode::Check : FormatMode::Apply;
		request.DestructiveActionConfirmed = m_settings.ConfirmClean();

		if (operationId == "quality.format")
		{
			const MaintenanceOperationPlan plan = PlanMaintenanceOperation(operationId.toStdString(), request);
			const bool formatNeedsAttention = plan.Toolchain.ClangFormatPath.empty() || plan.FormatSourceFiles.empty();
			QVBoxLayout* maintenanceLayout = AddDetailsGroup(
			    layout,
			    formatNeedsAttention ? "Action Dependencies - Needs action" : "Action Dependencies - Ready",
			    "Formatting depends on clang-format being installed and source files being discoverable in Engine/ and Projects/.",
			    false);
			AddStatusRow(
			    *maintenanceLayout,
			    "clang-format",
			    plan.Toolchain.ClangFormatPath.empty() ? "Missing" : "Ready",
			    plan.Toolchain.ClangFormatPath.empty() ? "clang-format was not found." : "clang-format is available.",
			    plan.Toolchain.ClangFormatPath.empty() ? "warning" : "ok",
			    CreateActionDependencyActions("toolchain.check", "Verify Host Environment"));
			AddStatusRow(
			    *maintenanceLayout,
			    "Source files",
			    plan.FormatSourceFiles.empty() ? "None found" : "Ready",
			    QString("Discovered %1 source files eligible for formatting.").arg(plan.FormatSourceFiles.size()),
			    plan.FormatSourceFiles.empty() ? "warning" : "ok");
			return;
		}

		if (operationId == "workspace.clean")
		{
			QVBoxLayout* maintenanceLayout = AddDetailsGroup(
			    layout,
			    m_settings.ConfirmClean() ? "Action Dependencies - Ready" : "Action Dependencies - Confirmation required",
			    "Cleaning generated outputs does not require the build toolchain, but destructive scopes still require explicit confirmation.",
			    false);
			AddStatusRow(
			    *maintenanceLayout,
			    "Confirmation",
			    m_settings.ConfirmClean() ? "Enabled" : "Required on run",
			    m_settings.ConfirmClean() ? "Clean confirmation is enabled in settings." : "The launcher will ask for confirmation before destructive clean actions run.",
			    m_settings.ConfirmClean() ? "ok" : "warning");
		}
	}
}

