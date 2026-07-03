#include "LauncherPrerequisitePrompts.h"

#include "LauncherOperationRequestFactory.h"
#include "LauncherProjectModel.h"
#include "LauncherSettings.h"
#include "SparkleLauncher/BuildWorkspaceOperations.h"
#include "SparkleLauncher/CookOperations.h"
#include "SparkleLauncher/LaunchOperations.h"

#include <QtCore/QRegularExpression>
#include <QtCore/QStringList>
#include <QtCore/QProcess>
#include <QtWidgets/QMessageBox>

namespace SparkleLauncher
{
	namespace
	{
		LauncherPrerequisiteDecision ReadyDecision()
		{
			LauncherPrerequisiteDecision decision;
			decision.Result = LauncherPrerequisiteDecision::Kind::Ready;
			return decision;
		}

		LauncherPrerequisiteDecision BlockedDecision(QString statusMessage = {})
		{
			LauncherPrerequisiteDecision decision;
			decision.Result = LauncherPrerequisiteDecision::Kind::Blocked;
			decision.StatusMessage = std::move(statusMessage);
			return decision;
		}

		LauncherPrerequisiteDecision PrerequisiteDecision(LauncherOperationRequest request, QString title)
		{
			LauncherPrerequisiteDecision decision;
			decision.Result = LauncherPrerequisiteDecision::Kind::RunPrerequisite;
			decision.Request = std::move(request);
			decision.Title = std::move(title);
			return decision;
		}

		QStringList ToQStringList(const std::vector<std::string>& messages)
		{
			QStringList result;
			for (const std::string& message : messages)
			{
				result.push_back(QString::fromStdString(message));
			}
			return result;
		}

		LaunchOperationRequest ToLaunchPlanRequest(const LauncherOperationRequest& request)
		{
			LaunchOperationRequest launchRequest;
			launchRequest.RepositoryRoot = request.RepositoryRoot;
			launchRequest.ProjectId = request.ProjectId.toStdString();
			launchRequest.EditorProfile = request.EditorProfile.toStdString();
			launchRequest.RuntimeProfile = request.RuntimeProfile.toStdString();
			launchRequest.Target = request.LaunchTarget.toStdString();
			launchRequest.StartupLevel = request.LaunchStartupLevel.toStdString();
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
			return launchRequest;
		}

		CookOperationRequest ToCookPlanRequest(const LauncherOperationRequest& request)
		{
			CookOperationRequest cookRequest;
			cookRequest.RepositoryRoot = request.RepositoryRoot;
			cookRequest.ProjectId = request.ProjectId.toStdString();
			cookRequest.RuntimeProfile = request.RuntimeProfile.toStdString();
			cookRequest.Mode = request.ForceRecook ? CookMode::Force : CookMode::Incremental;
			cookRequest.ForceRecookConfirmed = request.ConfirmForceRecook;
			for (const QString& part : request.ShaderPackages.split(QRegularExpression("[,;\\n]"), Qt::SkipEmptyParts))
			{
				const QString trimmed = part.trimmed();
				if (!trimmed.isEmpty())
				{
					cookRequest.ShaderPackages.push_back(trimmed.toStdString());
				}
			}
			return cookRequest;
		}

		bool ConfirmPrerequisitePrompt(QWidget* parent, const QString& title, const QString& promptAction, const QStringList& readiness)
		{
			const QMessageBox::StandardButton result = QMessageBox::question(
			    parent,
			    title,
			    promptAction + (readiness.isEmpty() ? QString() : "\n\n" + readiness.join('\n')),
			    QMessageBox::Ok | QMessageBox::Cancel,
			    QMessageBox::Ok);
			return result == QMessageBox::Ok;
		}
	}

	LauncherPrerequisiteDecision ResolveWorkspacePrerequisitePrompt(
	    QWidget* parent,
	    const std::filesystem::path& repositoryRoot,
	    const LauncherProjectModel& projectModel,
	    const LauncherSettings& settings,
	    const QString& operationId)
	{
		const BuildWorkspaceOperationRequest request = BuildWorkspacePlanRequest(repositoryRoot, projectModel, settings);
		const BuildWorkspaceOperationPlan plan = PlanBuildWorkspaceOperation(operationId.toStdString(), request);
		if (plan.CanRun)
		{
			return ReadyDecision();
		}

		QString prerequisiteOperationId;
		QString promptTitle;
		QString promptAction;
		if (!plan.Toolchain.RequiredToolsAvailable)
		{
			prerequisiteOperationId = "toolchain.check";
			promptTitle = "Sync Diagnostics";
			promptAction = "Required host prerequisites are missing. Run a sync diagnostics check now?";
		}
		else if ((operationId == "workspace.open-ide" || operationId == "workspace.build-all" || operationId == "launcher.build.self" || operationId.startsWith("project.build") || operationId == "cook.tools.prepare") && !plan.Freshness.Current)
		{
			prerequisiteOperationId = "workspace.generate-build-files";
			promptTitle = "Generate Build Files";
			promptAction = "Generated build files are not current. Run Generate Build Files now?";
		}
		else if (operationId == "workspace.open-ide")
		{
			prerequisiteOperationId = "toolchain.check";
			promptTitle = "Sync Diagnostics";
			promptAction = QString("%1 is not currently available. Run a sync diagnostics check now and verify the Visual Studio, Qt, and optional ClangCL toolchain?").arg(ResolveSelectedWorkspaceIdeName(settings));
		}
		else
		{
			QMessageBox::information(
			    parent,
			    "Workflow Blocked",
			    "This workspace workflow is currently blocked.\n\n" + ToQStringList(plan.ReadinessMessages).join('\n'));
			return BlockedDecision();
		}

		if (!ConfirmPrerequisitePrompt(parent, "Prerequisite Missing", promptAction, ToQStringList(plan.ReadinessMessages)))
		{
			return BlockedDecision();
		}

		return PrerequisiteDecision(
		    BuildLauncherOperationRequest(repositoryRoot, projectModel, settings, prerequisiteOperationId),
		    promptTitle);
	}

	LauncherPrerequisiteDecision ResolveLaunchPrerequisitePrompt(
	    QWidget* parent,
	    const std::filesystem::path& repositoryRoot,
	    const LauncherProjectModel& projectModel,
	    const LauncherSettings& settings,
	    const QString& operationId)
	{
		const LauncherOperationRequest request = BuildLauncherOperationRequest(repositoryRoot, projectModel, settings, operationId);
		const LaunchOperationPlan plan = PlanLaunchOperation(operationId.toStdString(), ToLaunchPlanRequest(request));
		if (plan.CanRun)
		{
			return ReadyDecision();
		}

		QStringList readiness;
		for (const std::string& message : plan.ReadinessMessages)
		{
			readiness.push_back(QString::fromStdString(message));
		}

		const bool executableMissing = !plan.Readiness.ExecutableReady;
		const bool cookedMeshesMissing = !plan.Readiness.CookedMeshesReady;
		const bool cookedTexturesMissing = !plan.Readiness.CookedTexturesReady;
		const bool cookedShadersMissing = !plan.Readiness.CookedShadersReady;

		QString prerequisiteOperationId;
		QString promptTitle;
		QString promptAction;
		if (executableMissing)
		{
			const bool runtimeTarget = request.LaunchTarget == "runtime";
			prerequisiteOperationId = runtimeTarget ? "project.build.runtime" : "project.build.editor";
			promptTitle = runtimeTarget ? "Build Runtime" : "Build Editor";
			promptAction = "The executable is missing. Start " + promptTitle + " now?";
		}
		else if (cookedMeshesMissing || cookedTexturesMissing || cookedShadersMissing)
		{
			const int missingCount = static_cast<int>(cookedMeshesMissing) + static_cast<int>(cookedTexturesMissing) + static_cast<int>(cookedShadersMissing);
			if (missingCount == 1)
			{
				if (cookedMeshesMissing)
				{
					prerequisiteOperationId = "cook.assets";
					promptTitle = "Cook Scenes And Meshes";
					promptAction = "Cooked scenes and meshes are missing. Start Cook Scenes And Meshes now?";
				}
				else if (cookedTexturesMissing)
				{
					prerequisiteOperationId = "cook.textures";
					promptTitle = "Cook Textures";
					promptAction = "Cooked textures are missing. Start Cook Textures now?";
				}
				else
				{
					prerequisiteOperationId = "cook.shaders";
					promptTitle = "Cook Shaders";
					promptAction = "Cooked shaders are missing. Start Cook Shaders now?";
				}
			}
			else
			{
				prerequisiteOperationId = "cook.project";
				promptTitle = "Cook All";
				promptAction = "Multiple cooked asset sets are missing. Start Cook All now?";
			}
		}
		else
		{
			return ReadyDecision();
		}

		if (!ConfirmPrerequisitePrompt(parent, "Launch Prerequisite Missing", promptAction, readiness))
		{
			return BlockedDecision("Launch canceled");
		}

		return PrerequisiteDecision(
		    BuildLauncherOperationRequest(repositoryRoot, projectModel, settings, prerequisiteOperationId),
		    promptTitle);
	}

	LauncherPrerequisiteDecision ResolveCookPrerequisitePrompt(
	    QWidget* parent,
	    const std::filesystem::path& repositoryRoot,
	    const LauncherProjectModel& projectModel,
	    const LauncherSettings& settings,
	    const QString& operationId)
	{
		const LauncherOperationRequest request = BuildLauncherOperationRequest(repositoryRoot, projectModel, settings, operationId);
		const CookOperationPlan plan = PlanCookOperation(operationId.toStdString(), ToCookPlanRequest(request));
		if (plan.CanRun)
		{
			return ReadyDecision();
		}

		bool workspaceMissing = false;
		bool cookToolsMissing = false;
		bool cookToolRuntimeMissing = false;
		bool dependencyGroupDisabled = false;
		QStringList readiness;
		for (const std::string& message : plan.ReadinessMessages)
		{
			const QString readinessMessage = QString::fromStdString(message);
			readiness.push_back(readinessMessage);
			workspaceMissing = workspaceMissing || readinessMessage.contains("Run Generate Build Files first", Qt::CaseInsensitive);
			cookToolsMissing = cookToolsMissing || readinessMessage.contains("run Build Cooking Tools first", Qt::CaseInsensitive);
			cookToolRuntimeMissing = cookToolRuntimeMissing || readinessMessage.contains("runtime dependency is missing", Qt::CaseInsensitive) ||
			    readinessMessage.contains("runtime support bundle is incomplete", Qt::CaseInsensitive);
			dependencyGroupDisabled = dependencyGroupDisabled || readinessMessage.contains("disabled in this workspace configuration", Qt::CaseInsensitive) ||
			    readinessMessage.contains("No cook tool groups are enabled", Qt::CaseInsensitive);
		}

		if (dependencyGroupDisabled)
		{
			QMessageBox::information(
			    parent,
			    "Cook Workflow Disabled",
			    "This cook workflow is disabled by the current workspace dependency-group configuration.\n\n" + readiness.join('\n'));
			return BlockedDecision();
		}

		QString prerequisiteOperationId;
		QString promptTitle;
		QString promptAction;
		if (workspaceMissing)
		{
			prerequisiteOperationId = "workspace.generate-build-files";
			promptTitle = "Generate Build Files";
			promptAction = "Generated build files are not current. Run Generate Build Files now?";
		}
		else if (cookToolsMissing || cookToolRuntimeMissing)
		{
			prerequisiteOperationId = "cook.tools.prepare";
			promptTitle = "Build Cooking Tools";
			promptAction = cookToolRuntimeMissing ?
			                   "Required cooking tool runtime support files are missing. Run Build Cooking Tools now?" :
			                   "Required cooking tools are missing. Run Build Cooking Tools now?";
		}
		else
		{
			return ReadyDecision();
		}

		if (!ConfirmPrerequisitePrompt(parent, "Cook Prerequisite Missing", promptAction, readiness))
		{
			return BlockedDecision();
		}

		return PrerequisiteDecision(
		    BuildLauncherOperationRequest(repositoryRoot, projectModel, settings, prerequisiteOperationId),
		    promptTitle);
	}
}
