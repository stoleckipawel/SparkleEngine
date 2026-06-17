#include "LauncherMainWindow.h"

#include "LauncherActionWidgets.h"
#include "LauncherBackend.h"
#include "LauncherDependencyUiModel.h"
#include "LauncherOperationRequestFactory.h"
#include "LauncherPrerequisitePrompts.h"
#include "LauncherProjectModel.h"
#include "LauncherSettings.h"
#include "LauncherUiDesign.h"
#include "LauncherWorkflowCatalog.h"

#include "SparkleLauncher/BuildWorkspaceOperations.h"
#include "SparkleLauncher/CookOperations.h"
#include "SparkleLauncher/LauncherPaths.h"
#include "SparkleLauncher/LaunchOperations.h"
#include "SparkleLauncher/MaintenanceOperations.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QProcess>
#include <QtCore/QRegularExpression>
#include <QtCore/QUrl>
#include <QtGui/QClipboard>
#include <QtGui/QDesktopServices>
#include <QtGui/QGuiApplication>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QToolButton>

#include <filesystem>
#include <utility>

namespace SparkleLauncher
{
	void LauncherMainWindow::RunSelectedOperation()
	{
		if (m_selectedOperationId.isEmpty())
		{
			if (m_operationOutput != nullptr)
			{
				m_operationOutput->setPlainText("Choose a workflow before running.");
			}
			return;
		}

		if (m_selectedOperationId == LauncherHomeOperationId())
		{
			return;
		}

		if (OperationNeedsProject(m_selectedOperationId) && m_projectModel.SelectedProjectId().isEmpty())
		{
			const QString message = "No project discovered. Confirm this is a Sparkle repository or package root with Projects/<Project> markers.";
			if (m_operationOutput != nullptr)
			{
				m_operationOutput->setPlainText(message);
			}
			return;
		}

		if ((m_selectedOperationId == "workspace.sync-source-tiers" || m_selectedOperationId == "workspace.generate-build-files" || m_selectedOperationId == "workspace.open-ide" ||
		     m_selectedOperationId == "workspace.build-all" || m_selectedOperationId == "launcher.build.self" || m_selectedOperationId.startsWith("project.build") || m_selectedOperationId == "cook.tools.prepare") &&
		    !OfferWorkspacePrerequisiteOperation(m_selectedOperationId))
		{
			return;
		}

		if (m_selectedOperationId.startsWith("cook.") && m_selectedOperationId != "cook.tools.prepare" && !OfferCookPrerequisiteOperation(m_selectedOperationId))
		{
			return;
		}

		if (FindLaunchOperationDefinition(m_selectedOperationId.toStdString()).has_value() && !OfferLaunchPrerequisiteOperation(m_selectedOperationId))
		{
			return;
		}

		LauncherOperationRequest request = BuildLauncherOperationRequest(m_repositoryRoot, m_projectModel, m_settings, m_selectedOperationId);
		if (!ConfirmRunRequest(request))
		{
			return;
		}

		const QString title = DisplayNameForOperation(m_selectedOperationId);
		StartOperation(std::move(request), title);
	}

	void LauncherMainWindow::CleanSelectedOperation()
	{
		if (m_selectedOperationId.isEmpty())
		{
			return;
		}

		if (m_selectedOperationId == "workspace.clean")
		{
			LauncherOperationRequest request = BuildScopedCleanOperationRequest(m_repositoryRoot, m_projectModel, m_settings, "pristine");
			if (!ConfirmRunRequest(request))
			{
				return;
			}

			StartOperation(std::move(request), "Clean All");
			return;
		}

		if (!SupportsActionSpecificClean(m_selectedOperationId))
		{
			return;
		}

		LauncherOperationRequest request = BuildActionCleanOperationRequest(
		    m_repositoryRoot,
		    m_projectModel,
		    m_settings,
		    std::filesystem::path(QCoreApplication::applicationFilePath().toStdString()),
		    m_selectedOperationId);
		if (request.CleanTargets.isEmpty())
		{
			const QString message = OperationNeedsProject(m_selectedOperationId) && m_projectModel.SelectedProjectId().isEmpty() ?
			                            "Select a project before cleaning this workflow's generated outputs." :
			                            "No generated outputs were resolved for this workflow.";
			if (m_operationOutput != nullptr)
			{
				m_operationOutput->setPlainText(message);
			}
			return;
		}

		if (!ConfirmRunRequest(request))
		{
			return;
		}

		StartOperation(std::move(request), "Clean " + DisplayNameForOperation(m_selectedOperationId));
	}


	QWidget* LauncherMainWindow::CreateTrackedDependencyActions(const ThirdPartyDependencyUiEntry& dependency)
	{
		QToolButton* button = CreateLauncherOverflowActionButton(
		    this,
		    dependency.Label + " actions",
		    "Dependency actions",
		    {
		        LauncherActionMenuEntry{
		            "Refresh this cache",
		            [this, dependency]() { TriggerDependencyRegenerate(dependency); }},
		        LauncherActionMenuEntry{
		            "Clean this cache",
		            [this, dependency]() { TriggerDependencyClean(dependency); }},
		    });
		RegisterFocusable(button);
		return button;
	}

	QWidget* LauncherMainWindow::CreateDisabledSourceTierActions(const DependencyGroupUiEntry& group)
	{
		QVector<LauncherActionMenuEntry> entries;
		if (!group.ConfigureOption.isEmpty())
		{
			entries.push_back(LauncherActionMenuEntry{
			    "Copy enable option",
			    [this, option = group.ConfigureOption]() {
				    QGuiApplication::clipboard()->setText("-D" + option + "=ON");
			    }});
		}
		entries.push_back(LauncherActionMenuEntry{
		    "Open dependency guide",
		    [this]() {
			    OpenLocalPath(m_repositoryRoot / "docs" / "dependency-capability-tiers.md");
		    }});
		entries.push_back(LauncherActionMenuEntry{
		    "Open reconfigure workflow",
		    [this]() {
			    TriggerActionDependencyRegenerate("workspace.generate-build-files", "Open Reconfigure Workflow", true);
		    }});

		QToolButton* button = CreateLauncherOverflowActionButton(
		    this,
		    group.Label + " configuration actions",
		    "Tier actions",
		    entries);
		RegisterFocusable(button);
		return button;
	}

	QWidget* LauncherMainWindow::CreateActionDependencyActions(
	    const QString& actionId,
	    const QString& actionTitle,
	    const QString& cleanScope,
	    const QString& cleanTitle,
	    bool navigateInsteadOfRun)
	{
		QVector<LauncherActionMenuEntry> entries;
		entries.push_back(LauncherActionMenuEntry{
		    actionTitle,
		    [this, actionId, actionTitle, navigateInsteadOfRun]() {
			    TriggerActionDependencyRegenerate(actionId, actionTitle, navigateInsteadOfRun);
		    }});
		if (!cleanScope.isEmpty())
		{
			entries.push_back(LauncherActionMenuEntry{
			    "Clean",
			    [this, cleanScope, cleanTitle]() {
				    TriggerActionDependencyClean(cleanScope, cleanTitle);
			    }});
		}
		QToolButton* button = CreateLauncherOverflowActionButton(
		    this,
		    actionTitle + " actions",
		    "Dependency actions",
		    entries);
		RegisterFocusable(button);
		return button;
	}

	void LauncherMainWindow::OpenLocalPath(const std::filesystem::path& path)
	{
		std::error_code errorCode;
		const bool isAvailable =
		    std::filesystem::exists(path, errorCode) ||
		    std::filesystem::is_directory(path, errorCode);
		if (!isAvailable)
		{
			QMessageBox::information(this, "Target Not Available", "This file or folder is not available yet.");
			return;
		}

		QDesktopServices::openUrl(QUrl::fromLocalFile(QString::fromStdString(path.string())));
	}

	void LauncherMainWindow::TriggerActionDependencyClean(const QString& cleanScope, const QString& cleanTitle)
	{
		LauncherOperationRequest request = BuildScopedCleanOperationRequest(m_repositoryRoot, m_projectModel, m_settings, cleanScope);
		if (!ConfirmRunRequest(request))
		{
			return;
		}

		StartOperation(std::move(request), cleanTitle);
	}

	void LauncherMainWindow::TriggerActionDependencyRegenerate(const QString& actionId, const QString& actionTitle, bool navigateInsteadOfRun)
	{
		if (navigateInsteadOfRun)
		{
			SetSelectedOperation(actionId);
			return;
		}

		LauncherOperationRequest request = BuildLauncherOperationRequest(m_repositoryRoot, m_projectModel, m_settings, actionId);
		if (!ConfirmRunRequest(request))
		{
			return;
		}

		StartOperation(std::move(request), actionTitle);
	}

	void LauncherMainWindow::TriggerDependencyClean(const ThirdPartyDependencyUiEntry& dependency)
	{
		LauncherOperationRequest request = BuildDependencyCleanOperationRequest(m_repositoryRoot, m_projectModel, m_settings, dependency);
		if (!ConfirmRunRequest(request))
		{
			return;
		}

		StartOperation(std::move(request), "Clean " + dependency.Label);
	}

	void LauncherMainWindow::TriggerDependencyRegenerate(const ThirdPartyDependencyUiEntry& dependency)
	{
		const QMessageBox::StandardButton regenerateResult = QMessageBox::question(
		    this,
		    "Refresh Dependency Cache",
		    QStringLiteral("This will clean only the cached %1 source folder first, then run Prepare Workspace to refill any missing enabled package content. Continue?").arg(dependency.Label),
		    QMessageBox::Ok | QMessageBox::Cancel,
		    QMessageBox::Ok);
		if (regenerateResult != QMessageBox::Ok)
		{
			return;
		}

		LauncherOperationRequest cleanRequest = BuildDependencyCleanOperationRequest(m_repositoryRoot, m_projectModel, m_settings, dependency);
		if (!ConfirmRunRequest(cleanRequest))
		{
			return;
		}

		LauncherOperationRequest setupRequest = BuildDependencyRegenerateOperationRequest(m_repositoryRoot, m_projectModel, m_settings);
		const QString title = "Regenerate " + dependency.Label;
		const QString cleanTitle = "Clean cache before " + title;
		const QString runId = QStringLiteral("run-%1").arg(m_nextRunIndex + 1, 4, 10, QChar('0'));
		PendingFollowUpOperation followUp;
		followUp.Request = std::move(setupRequest);
		followUp.Title = title;
		m_pendingFollowUpOperations.insert(runId, std::move(followUp));
		StartOperation(std::move(cleanRequest), cleanTitle);
	}


	const LauncherOperationDescriptor* LauncherMainWindow::FindOperationDescriptor(const QString& operationId) const
	{
		for (const LauncherOperationDescriptor& operation : m_backend.Operations())
		{
			if (operation.Id == operationId)
			{
				return &operation;
			}
		}
		return nullptr;
	}

	QString LauncherMainWindow::DisplayNameForOperation(const QString& operationId) const
	{
		const QString overrideName = LauncherOperationDisplayNameOverride(operationId);
		if (!overrideName.isEmpty())
		{
			return overrideName;
		}
		const LauncherOperationDescriptor* operation = FindOperationDescriptor(operationId);
		return operation == nullptr ? operationId : operation->DisplayName;
	}

	bool LauncherMainWindow::OperationNeedsProject(const QString& operationId) const
	{
		if (operationId == "workspace.clean")
		{
			return m_settings.CleanScope().contains("selected-cooked");
		}

		return operationId.startsWith("project.") || operationId.startsWith("cook.");
	}

	bool LauncherMainWindow::OperationNeedsConfirmation(const QString& operationId) const
	{
		if (operationId.startsWith("cook."))
		{
			return m_settings.ForceRecook() && !m_settings.ConfirmForceRecook();
		}
		if (operationId == "workspace.clean")
		{
			return false;
		}

		return false;
	}

	QString LauncherMainWindow::FailureRecoveryHint(const QString& operationId, const QString& statusText) const
	{
		if (OperationNeedsProject(operationId) && m_projectModel.SelectedProjectId().isEmpty())
		{
			return "No project is selected. Confirm the repository/package root contains Projects/<Project> markers, then regenerate project files if rebuilding from source.";
		}
		if (operationId.startsWith("cook.") && OperationNeedsConfirmation(operationId))
		{
			return "Enable Confirm clean cook, then retry.";
		}
		if ((operationId == "workspace.sync-source-tiers" || operationId == "workspace.generate-build-files") &&
		    (statusText.contains("dxcapi.h", Qt::CaseInsensitive) || statusText.contains("slang", Qt::CaseInsensitive) ||
		     statusText.contains("VULKAN_SDK", Qt::CaseInsensitive) || statusText.contains("ShaderCompiler", Qt::CaseInsensitive)))
		{
			return "Install or expose the Vulkan SDK so Vulkan-backed editor/runtime builds and the enabled shader compiler tier can resolve DXC, Slang, and Vulkan headers, then open Sync and retry.";
		}
		if ((operationId == "workspace.sync-source-tiers" || operationId == "workspace.generate-build-files") &&
		    (statusText.contains("NVIDIA Streamline SDK", Qt::CaseInsensitive) || statusText.contains("sl.interposer.lib", Qt::CaseInsensitive) ||
		     statusText.contains("sl.dlss.dll", Qt::CaseInsensitive) || statusText.contains("nvngx_dlss.dll", Qt::CaseInsensitive)))
		{
			return "Sync fetches the NVIDIA Streamline SDK automatically. If this still fails after retry, verify network access to GitHub releases, then clean the source dependency cache and run Prepare Workspace again.";
		}
		if ((operationId == "workspace.sync-source-tiers" || operationId == "workspace.generate-build-files") &&
		    (statusText.contains("NVAPI", Qt::CaseInsensitive) || statusText.contains("nvapi.h", Qt::CaseInsensitive) ||
		     statusText.contains("nvapi64.lib", Qt::CaseInsensitive)))
		{
			return "Sync fetches NVAPI automatically. If this still fails after retry, clean the source dependency cache and rerun Prepare Workspace so the launcher can re-download a clean NVIDIA SDK checkout.";
		}
		if ((operationId == "workspace.sync-source-tiers" || operationId == "workspace.generate-build-files") &&
		    (statusText.contains("FetchContent", Qt::CaseInsensitive) || statusText.contains("not a git repository", Qt::CaseInsensitive) ||
		     statusText.contains("source directory is missing", Qt::CaseInsensitive) || statusText.contains("nvapi.h", Qt::CaseInsensitive)))
		{
			return "Run Clean Source Dependency Cache, then retry Prepare Workspace. The launcher will repopulate stale dependency checkouts automatically.";
		}
		if (operationId.startsWith("project.build") || statusText.contains("cmake", Qt::CaseInsensitive) || statusText.contains("MSBuild", Qt::CaseInsensitive) || statusText.contains("tool", Qt::CaseInsensitive))
		{
			return "Open Sync, review the missing machine prerequisites, then retry this workflow.";
		}
		if (statusText.contains("Rider", Qt::CaseInsensitive))
		{
			return "Install Rider or switch the IDE selector back to Visual Studio, then retry.";
		}
		if (statusText.contains("disabled in this workspace configuration", Qt::CaseInsensitive) ||
		    statusText.contains("No cook tool groups are enabled", Qt::CaseInsensitive))
		{
			return "This workflow is disabled by the current dependency-group configuration. Reconfigure the workspace with the matching group enabled, then sync and build again.";
		}
		if (statusText.contains("shader package", Qt::CaseInsensitive) || statusText.contains("shader", Qt::CaseInsensitive))
		{
			return "Run Cook > Cook Shaders, then retry this workflow.";
		}
		if (statusText.contains("executable is missing", Qt::CaseInsensitive) || statusText.contains("missing", Qt::CaseInsensitive))
		{
			const bool runtimeLaunch = operationId == "project.open.runtime" || ((operationId == "project.run" || operationId == "project.run.smoke") && m_settings.LaunchTarget() == "runtime");
			if ((operationId == "project.open.runtime" || operationId == "project.run" || operationId == "project.run.smoke") && runtimeLaunch)
			{
				return "Run Build > Build Runtime, then retry this workflow.";
			}
			if (FindLaunchOperationDefinition(operationId.toStdString()).has_value())
			{
				return "Run Build > Build Editor, then retry this workflow.";
			}
		}

		if (operationId.startsWith("cook."))
		{
			return "Review the output below. If tools or cooked inputs are missing, run Build Cooking Tools before retrying.";
		}

		if (FindLaunchOperationDefinition(operationId.toStdString()).has_value())
		{
			return "Review the output below. If package binaries are missing, use a complete runtime package; if source artifacts are missing, build the matching target before retrying.";
		}

		return "Review the output below, adjust the selected options, then retry.";
	}

	bool LauncherMainWindow::ConfirmRunRequest(LauncherOperationRequest& request) const
	{
		const bool cleanRequested = request.OperationId == "workspace.clean";
		const bool customCleanRequested = cleanRequested && !request.CleanTargets.isEmpty();
		const bool destructiveRequested = request.ForceRecook || cleanRequested;
		if (!destructiveRequested)
		{
			return true;
		}
		if (request.ForceRecook && !request.ConfirmForceRecook)
		{
			QMessageBox::warning(
			    const_cast<LauncherMainWindow*>(this),
			    "Confirmation Required",
			    "Enable Confirm clean cook before removing cooked outputs.");
			return false;
		}
		if (cleanRequested && !request.ConfirmClean)
		{
			QStringList scopeNames;
			if (customCleanRequested)
			{
				for (const LauncherCleanTarget& target : request.CleanTargets)
				{
					scopeNames.push_back(target.DisplayName + "\n" + target.Path);
				}
			}
			else
			{
				for (const QString& scopeValue : request.CleanScope.split(QRegularExpression("[,;\\n]"), Qt::SkipEmptyParts))
				{
					scopeNames.push_back(CleanScopeDisplayName(scopeValue));
				}
			}

			QString message = customCleanRequested ? "Generated outputs to clean:\n\n" + scopeNames.join("\n\n") : "Clean scopes:\n" + scopeNames.join('\n');
			if (!request.ProjectId.isEmpty())
			{
				message += "\nProject: " + request.ProjectId;
			}
			message += customCleanRequested ? "\n\nThis removes only the generated outputs mapped to the selected action. Continue?" :
			                                 "\n\nThis removes generated files for the selected scope. Continue?";
			const QMessageBox::StandardButton result = QMessageBox::question(
			    const_cast<LauncherMainWindow*>(this),
			    customCleanRequested ? "Confirm Action Clean" : "Confirm Clean Workspace",
			    message,
			    QMessageBox::Ok | QMessageBox::Cancel,
			    QMessageBox::Cancel);
			if (result != QMessageBox::Ok)
			{
				return false;
			}

			request.ConfirmClean = true;
			return true;
		}

		const QMessageBox::StandardButton result = QMessageBox::question(
		    const_cast<LauncherMainWindow*>(this),
		    "Confirm Clean Cook",
		    "This workflow will remove cooked outputs before cooking. Continue?",
		    QMessageBox::Yes | QMessageBox::No,
		    QMessageBox::No);
		return result == QMessageBox::Yes;
	}

	void LauncherMainWindow::PromptForLauncherRestart()
	{
		const QMessageBox::StandardButton result = QMessageBox::question(
		    this,
		    "Launcher Rebuilt",
		    "Sparkle Launcher was rebuilt successfully. Restart now to run the new binary?",
		    QMessageBox::Yes | QMessageBox::No,
		    QMessageBox::Yes);
		if (result != QMessageBox::Yes)
		{
			return;
		}

		const std::filesystem::path relaunchedExecutablePath =
		    GetLauncherArtifactDirectory(m_repositoryRoot, m_settings.EditorProfile().toStdString()) /
		    std::filesystem::path(QCoreApplication::applicationFilePath().toStdString()).filename();
		const QString executablePath = QString::fromStdString(relaunchedExecutablePath.string());
		const bool started = QProcess::startDetached(executablePath, {});
		if (!started)
		{
			QMessageBox::warning(
			    this,
			    "Restart Failed",
			    "The rebuilt launcher is ready, but the restart command could not be started.");
			return;
		}

		QCoreApplication::quit();
	}

	bool LauncherMainWindow::OfferWorkspacePrerequisiteOperation(const QString& operationId)
	{
		LauncherPrerequisiteDecision decision =
		    ResolveWorkspacePrerequisitePrompt(this, m_repositoryRoot, m_projectModel, m_settings, operationId);
		if (decision.Result == LauncherPrerequisiteDecision::Kind::Ready)
		{
			return true;
		}
		if (decision.Result == LauncherPrerequisiteDecision::Kind::Blocked)
		{
			if (!decision.StatusMessage.isEmpty())
			{
				QMessageBox::information(this, "Workflow Not Ready", decision.StatusMessage);
			}
			return false;
		}
		if (!ConfirmRunRequest(decision.Request))
		{
			return false;
		}
		StartOperation(std::move(decision.Request), decision.Title);
		return false;
	}

	bool LauncherMainWindow::OfferLaunchPrerequisiteOperation(const QString& operationId)
	{
		LauncherPrerequisiteDecision decision =
		    ResolveLaunchPrerequisitePrompt(this, m_repositoryRoot, m_projectModel, m_settings, operationId);
		if (decision.Result == LauncherPrerequisiteDecision::Kind::Ready)
		{
			return true;
		}
		if (decision.Result == LauncherPrerequisiteDecision::Kind::Blocked)
		{
			if (!decision.StatusMessage.isEmpty())
			{
				QMessageBox::information(this, "Workflow Not Ready", decision.StatusMessage);
			}
			return false;
		}
		if (!ConfirmRunRequest(decision.Request))
		{
			return false;
		}
		StartOperation(std::move(decision.Request), decision.Title);
		return false;
	}

	bool LauncherMainWindow::OfferCookPrerequisiteOperation(const QString& operationId)
	{
		LauncherPrerequisiteDecision decision =
		    ResolveCookPrerequisitePrompt(this, m_repositoryRoot, m_projectModel, m_settings, operationId);
		if (decision.Result == LauncherPrerequisiteDecision::Kind::Ready)
		{
			return true;
		}
		if (decision.Result == LauncherPrerequisiteDecision::Kind::Blocked)
		{
			if (!decision.StatusMessage.isEmpty())
			{
				QMessageBox::information(this, "Workflow Not Ready", decision.StatusMessage);
			}
			return false;
		}
		if (!ConfirmRunRequest(decision.Request))
		{
			return false;
		}
		StartOperation(std::move(decision.Request), decision.Title);
		return false;
	}

	void LauncherMainWindow::StartOperation(LauncherOperationRequest request, const QString& title)
	{
		request.RunId = QStringLiteral("run-%1").arg(++m_nextRunIndex, 4, 10, QChar('0'));
		RegisterRun(request.RunId, title);
		m_backend.RunOperation(std::move(request));
	}

}
