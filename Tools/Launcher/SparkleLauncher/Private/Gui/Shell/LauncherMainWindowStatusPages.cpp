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
	static constexpr const char* kColorStateWarning = LauncherUi::Color::StateWarning;	void LauncherMainWindow::AddBuildEnvironmentStatus(QVBoxLayout& layout, const QString& operationId)
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
		const bool dependencyCacheReady = DirectoryHasEntries(dependencyCachePath);
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
				    group.Enabled ? CreateActionDependencyActions("workspace.sync-source-tiers", "Sync Source Tiers", "deps", "Clean Source Dependency Cache") :
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
			QVBoxLayout* hostDetailsLayout = AddDetailsGroup(
			    layout,
			    "Host Tool Details",
			    "Full installed-tool inventory and raw tool paths for diagnostics.",
			    !plan.Toolchain.RequiredToolsAvailable);
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
			QVBoxLayout* toolchainLayout = AddDetailsGroup(
			    layout,
			    plan.Toolchain.RequiredToolsAvailable ? "Action Dependencies - Ready" : "Action Dependencies - Needs action",
			    "Authoritative machine audit for local rebuilds, workspace generation, cook tooling, and IDE integration.",
			    false);
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
			const QString buildFilesLabel = operationId == "workspace.generate-build-files" ? "Generated build files" :
			                               operationId == "workspace.open-ide" ? "IDE build files" :
			                                                                        "Configured build files";
			const bool isGenerateBuildFilesWorkflow = operationId == "workspace.generate-build-files";
			const bool isSourceSyncWorkflow = operationId == "workspace.sync-source-tiers";
			const bool isOpenIdeWorkflow = operationId == "workspace.open-ide";
			const QString setupGroupTitle = isGenerateBuildFilesWorkflow ? "Build File Generation" :
			                                isOpenIdeWorkflow ? "IDE Launch Readiness" :
			                                                    "Source Tier Sync";
			const QString setupGroupDetail = isGenerateBuildFilesWorkflow ?
			                                     "Refresh generated CMake and IDE build-system files to match the selected generator, platform, toolset, and Qt kit." :
			                                 isOpenIdeWorkflow ?
			                                     "Open the selected IDE only after generated build files match the current toolchain selection." :
			                                     "Sync enabled source-tier capability groups and configure state. This does not install Visual Studio, Qt, CMake, Git, the Windows SDK, or other host prerequisites.";
			const QString buildFilesDetail = CombineStatusDetail(
			    CombineStatusDetail(QString::fromStdString(plan.Freshness.Summary), BuildFilesRecoveryHint(plan.Freshness)),
			    request.PreferredIde == WorkspaceIde::Rider ? QString("Repository root") : ToDisplayPath(m_repositoryRoot, plan.Freshness.SolutionPath));
			const QString cacheStatus = dependencyCacheReady ? "Ready" : "Will be created";
			const QString cacheDetail = dependencyCacheReady ?
			                               QString("Source dependency cache is available.") :
			                               QString("Source dependency cache will be populated when Sync Source Tiers runs.");
			const bool setupNeedsAttention = !plan.Toolchain.RequiredToolsAvailable || (isSourceSyncWorkflow && !dependencyCacheReady) || !plan.Freshness.Current;
			if (operationId == "workspace.sync-source-tiers")
			{
				AddSourceTierCards(
				    layout,
				    "Source Tier Workloads",
				    "Capability cards show what each tier unlocks. Individual dependency rows stay in details so Sync Source Tiers does not become a dependency log by default.",
				    true);
			}
			QVBoxLayout* workspaceLayout = AddDetailsGroup(
			    layout,
			    setupNeedsAttention ? setupGroupTitle + " - Needs action" : setupGroupTitle + " - Ready",
			    setupGroupDetail,
			    setupNeedsAttention);
			AddStatusRow(
			    *workspaceLayout,
			    buildFilesLabel,
			    isGenerateBuildFilesWorkflow ? (plan.Freshness.Current ? "Current" : "Will be refreshed") :
			                                (plan.Freshness.Current ? "Ready" : "Needs refresh"),
			    buildFilesDetail,
			    plan.Freshness.Current ? "ok" : "warning",
			    isGenerateBuildFilesWorkflow ? nullptr :
			                                  CreateActionDependencyActions("workspace.generate-build-files", "Generate Build Files", "build-tree", "Clean Build Files"));
			if (isSourceSyncWorkflow)
			{
				AddStatusRow(
				    *workspaceLayout,
				    "Local source dependency cache",
				    cacheStatus,
				    CombineStatusDetail(cacheDetail, FormatTrackedDependencySummary(dependencyCachePath)),
				    dependencyCacheReady ? "ok" : "warning",
				    CreateActionDependencyActions("workspace.sync-source-tiers", "Sync Source Tiers", "deps", "Clean Source Dependency Cache"));
			}
			if (!plan.Toolchain.RequiredToolsAvailable)
			{
				AddStatusRow(
				    *workspaceLayout,
				    "Required tools",
				    "Blocked",
				    RequiredToolProblemSummary(plan.Toolchain),
				    "bad",
				    CreateActionDependencyActions("toolchain.check", "Verify Host Environment"));
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
			            {"Bistro", "Bistro"},
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
		QVBoxLayout* smokeOptionsLayout = AddOptionGroup(layout, "Smoke Test Options", "Smoke-test controls for capture length and diagnostic behavior.");
		AddOptionField(
		    *smokeOptionsLayout,
		    "Frame limit",
		    CreateValueCombo(
		        {{"120 frames", ""}, {"60 frames", "60"}, {"300 frames", "300"}, {"600 frames", "600"}},
		        m_settings.SmokeFrameLimit(),
		        &LauncherSettings::SetSmokeFrameLimit));
		AddOptionField(
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
		AddOptionField(
		    *smokeOptionsLayout,
		    "Capture path",
		    CreateBoundLineEdit(
		        m_settings.SmokeCapturePath(),
		        "logs/smoke/scene-color.bmp",
		        "Optional scene-color capture path written by RHI smoke validation.",
		        &LauncherSettings::SetSmokeCapturePath));
		AddOptionCheckBox(
		    *smokeOptionsLayout,
		    CreateBoundCheckBox(
		        "Capture trace",
		        "Write smoke trace output.",
		        m_settings.SmokeTrace(),
		        &LauncherSettings::SetSmokeTrace));
		AddOptionCheckBox(
		    *smokeOptionsLayout,
		    CreateBoundCheckBox(
		        "Skip level switching",
		        "Do not switch levels during smoke.",
		        m_settings.SmokeSkipLevelSwitching(),
		        &LauncherSettings::SetSmokeSkipLevelSwitching));
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

