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
		const bool isSetupWorkflow =
		    operationId == "workspace.sync-code" || operationId == "workspace.generate-build-files" || operationId == "workspace.open-ide";
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
					    ToolchainStatusState(item.State, item.Required));
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
			                                                : (plan.Toolchain.VswherePath.empty() ? "warning" : "ok"));

			if (!isSourceSyncWorkflow)
			{
				AddStatusRow(
				    *machineLayout,
				    "Build files",
				    plan.Freshness.Current ? "Ready" : "Needs refresh",
				    CombineStatusDetail(QString::fromStdString(plan.Freshness.Summary), BuildFilesRecoveryHint(plan.Freshness)),
				    plan.Freshness.Current ? "ok" : "warning",
				    operationId == "workspace.open-ide" && !plan.Freshness.Current
				        ? CreateStatusActionButton("workspace.generate-build-files", "Generate", "Generate Build Files")
				        : nullptr);
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
			if (isSourceSyncWorkflow)
			{
				AddSyncDependencies(*machineLayout, false);
			}
			machineLayout->addSpacing(kSpaceSmall);
			machineLayout->addWidget(CreateSectionLabel("Optional dependencies"));
			addToolchainItems(false);
			if (isSourceSyncWorkflow)
			{
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
		const bool executableMissing = executableDetail.contains("missing", Qt::CaseInsensitive);
		const bool contentMissing = contentDetail.contains("missing", Qt::CaseInsensitive);
		const bool cookedMeshesMissing = cookedMeshesDetail.contains("missing", Qt::CaseInsensitive);
		const bool cookedTexturesMissing = cookedTexturesDetail.contains("missing", Qt::CaseInsensitive);
		const bool cookedShadersMissing = cookedShadersDetail.contains("missing", Qt::CaseInsensitive);

		QVBoxLayout* launchLayout = AddOptionGroup(layout, "Readiness", QString());
		if (launchRequest.GraphicsBackend == "vulkan" && toolchainStatus.VulkanSdkRoot.empty())
		{
			AddStatusRow(
			    *launchLayout,
			    "Graphics backend",
			    "Needs SDK",
			    "Vulkan was selected, but the Vulkan SDK is not available on this machine yet.",
			    "warning",
			    CreateStatusActionButton("workspace.sync-code", "Review", "Review Sync Code", true));
		}
		AddStatusRow(
		    *launchLayout,
		    runtimeTarget ? "Runtime executable" : "Editor executable",
		    executableMissing ? "Missing" : "Ready",
		    executableDetail,
		    executableMissing ? "warning" : "ok",
		    executableMissing ? CreateStatusActionButton(
		                            runtimeTarget ? "workspace.build.runtime" : "workspace.build.editor",
		                            "Build",
		                            runtimeTarget ? "Build Runtime" : "Build Editor")
		                      : nullptr);
		AddStatusRow(
		    *launchLayout,
		    "Content directory",
		    contentMissing ? "Missing" : "Ready",
		    contentDetail,
		    contentMissing ? "warning" : "ok",
		    contentMissing ? CreateStatusActionButton("workspace.sync-levels", "Sync", "Review Sync Levels", true) : nullptr);
		AddStatusRow(
		    *launchLayout,
		    "Cooked scene assets",
		    cookedMeshesMissing ? "Missing" : "Ready",
		    cookedMeshesDetail,
		    cookedMeshesMissing ? "warning" : "ok",
		    cookedMeshesMissing ? CreateStatusActionButton("cook.assets", "Cook", "Cook Scene Assets") : nullptr);
		AddStatusRow(
		    *launchLayout,
		    "Cooked textures",
		    cookedTexturesMissing ? "Missing" : "Ready",
		    cookedTexturesDetail,
		    cookedTexturesMissing ? "warning" : "ok",
		    cookedTexturesMissing ? CreateStatusActionButton("cook.textures", "Cook", "Cook Textures") : nullptr);
		AddStatusRow(
		    *launchLayout,
		    "Cooked shaders",
		    cookedShadersMissing ? "Missing" : "Ready",
		    cookedShadersDetail,
		    cookedShadersMissing ? "warning" : "ok",
		    cookedShadersMissing ? CreateStatusActionButton("cook.shaders", "Cook", "Cook Shaders") : nullptr);
	}

	void LauncherMainWindow::AddLaunchTargetOptions(QVBoxLayout& layout, const QString& title, const QString& detail)
	{
		QVBoxLayout* targetLayout = AddOptionGroup(layout, title, detail);
		AddOptionField(
		    *targetLayout,
		    "Application",
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
