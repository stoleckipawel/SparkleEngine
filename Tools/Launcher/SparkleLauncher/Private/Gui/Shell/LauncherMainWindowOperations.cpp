#include "LauncherMainWindow.h"

#include "LauncherActionWidgets.h"
#include "LauncherBackend.h"
#include "LauncherDependencyUiModel.h"
#include "LauncherOperationRequestFactory.h"
#include "LauncherContentModel.h"
#include "LauncherSettings.h"
#include "LauncherUiDesign.h"
#include "LauncherWorkflowCatalog.h"

#include "SparkleLauncher/BuildWorkspaceOperations.h"
#include "SparkleLauncher/CookOperations.h"
#include "SparkleLauncher/LauncherPaths.h"
#include "SparkleLauncher/MaintenanceOperations.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QProcess>
#include <QtCore/QRegularExpression>
#include <QtWidgets/QMessageBox>

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
			SyncAllLevels();
			return;
		}

		if (OperationNeedsContent(m_selectedOperationId) && m_contentModel.ContentId().isEmpty())
		{
			const QString message = "Repository content is unavailable. Confirm this is a complete Sparkle workspace.";
			if (m_operationOutput != nullptr)
			{
				m_operationOutput->setPlainText(message);
			}
			return;
		}

		if (m_selectedOperationId == "levels.sync")
		{
			SyncAllLevels();
			return;
		}

		LauncherOperationRequest request =
		    BuildLauncherOperationRequest(m_repositoryRoot, m_contentModel, m_settings, m_selectedOperationId);
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
		if (m_selectedOperationId == LauncherHomeOperationId())
		{
			CleanAllLevels();
			return;
		}

		if (m_selectedOperationId == "workspace.clean")
		{
			LauncherOperationRequest request = BuildScopedCleanOperationRequest(
			    m_repositoryRoot,
			    m_contentModel,
			    m_settings,
			    "clean-all",
			    std::filesystem::path(QCoreApplication::applicationFilePath().toStdString()));
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
		    m_contentModel,
		    m_settings,
		    std::filesystem::path(QCoreApplication::applicationFilePath().toStdString()),
		    m_selectedOperationId);
		if (request.CleanTargets.isEmpty())
		{
			const QString message = OperationNeedsContent(m_selectedOperationId) && m_contentModel.ContentId().isEmpty()
			    ? "Repository content is unavailable for this workflow's generated outputs."
			    : "No generated outputs were resolved for this workflow.";
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

	QPushButton* LauncherMainWindow::CreateStatusActionButton(
	    const QString& actionId,
	    const QString& actionLabel,
	    const QString& actionTitle,
	    bool navigateInsteadOfRun)
	{
		QPushButton* button = new QPushButton(this);
		ApplyStatusActionButtonPresentation(*button, actionLabel, "warning");
		button->setAccessibleName(actionTitle);
		button->setToolTip(actionTitle + ".");
		RegisterFocusable(button);
		connect(
		    button,
		    &QPushButton::clicked,
		    this,
		    [this, actionId, actionTitle, navigateInsteadOfRun]()
		    { TriggerActionDependencyRegenerate(actionId, actionTitle, navigateInsteadOfRun); });
		return button;
	}

	void LauncherMainWindow::TriggerActionDependencyRegenerate(
	    const QString& actionId,
	    const QString& actionTitle,
	    bool navigateInsteadOfRun)
	{
		if (navigateInsteadOfRun)
		{
			SetSelectedOperation(actionId);
			return;
		}

		LauncherOperationRequest request = BuildLauncherOperationRequest(m_repositoryRoot, m_contentModel, m_settings, actionId);
		if (!ConfirmRunRequest(request))
		{
			return;
		}

		StartOperation(std::move(request), actionTitle);
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

	bool LauncherMainWindow::OperationNeedsContent(const QString& operationId) const
	{
		if (operationId == "workspace.clean")
		{
			return m_settings.CleanScope().contains("cooked");
		}

		return operationId == LauncherHomeOperationId() || operationId == "levels.sync" || operationId == "levels.run"
		    || operationId.startsWith("workspace.build.") || operationId.startsWith("cook.");
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
		if (OperationNeedsContent(operationId) && m_contentModel.ContentId().isEmpty())
		{
			return "Repository content is unavailable. Confirm this is a complete Sparkle workspace, then regenerate build files if "
			       "rebuilding from source.";
		}
		if (operationId.startsWith("cook.") && OperationNeedsConfirmation(operationId))
		{
			return "Enable Confirm clean cook, then retry.";
		}
		if (operationId == "workspace.install-host-tool")
		{
			return statusText.contains("Visual Studio or MSBuild is running", Qt::CaseInsensitive)
			    ? "Close active Visual Studio, Rider build, MSBuild, and CMake processes, then retry Install."
			    : "Close active IDE builds, then retry Install. The launcher reports success only after detecting both clang-cl and its "
			      "Visual "
			      "Studio toolset.";
		}
		if ((operationId == "workspace.sync-code" || operationId == "workspace.generate-build-files")
		    && (statusText.contains("dxcapi.h", Qt::CaseInsensitive) || statusText.contains("slang", Qt::CaseInsensitive)
		        || statusText.contains("dxcompiler.dll", Qt::CaseInsensitive)
		        || statusText.contains("slang-compiler.dll", Qt::CaseInsensitive) || statusText.contains("VULKAN_SDK", Qt::CaseInsensitive)
		        || statusText.contains("ShaderCompiler", Qt::CaseInsensitive)))
		{
			return "Install or expose the Vulkan SDK so Vulkan-backed editor/runtime builds and the enabled shader compiler feature can "
			       "resolve DXC, Slang, Vulkan headers, and the required DXC/Slang runtime support bundle, then open Sync and retry.";
		}
		if ((operationId == "workspace.sync-code" || operationId == "workspace.generate-build-files")
		    && (statusText.contains("NVIDIA Streamline SDK", Qt::CaseInsensitive)
		        || statusText.contains("sl.interposer.lib", Qt::CaseInsensitive) || statusText.contains("sl.dlss.dll", Qt::CaseInsensitive)
		        || statusText.contains("sl.dlss_d.dll", Qt::CaseInsensitive) || statusText.contains("nvngx_dlss.dll", Qt::CaseInsensitive)
		        || statusText.contains("nvngx_dlssd.dll", Qt::CaseInsensitive)))
		{
			return "Sync fetches the NVIDIA Streamline SDK automatically. If this still fails after retry, verify network access to GitHub "
			       "releases, then clean the source dependency cache and run Sync Code again.";
		}
		if ((operationId == "workspace.sync-code" || operationId == "workspace.generate-build-files")
		    && (statusText.contains("NVAPI", Qt::CaseInsensitive) || statusText.contains("nvapi.h", Qt::CaseInsensitive)
		        || statusText.contains("nvapi64.lib", Qt::CaseInsensitive)))
		{
			return "Sync fetches NVAPI automatically. If this still fails after retry, clean the source dependency cache and rerun Sync "
			       "Code so the launcher can re-download a clean NVIDIA SDK checkout.";
		}
		if ((operationId == "workspace.sync-code" || operationId == "workspace.generate-build-files")
		    && (statusText.contains("FetchContent", Qt::CaseInsensitive) || statusText.contains("not a git repository", Qt::CaseInsensitive)
		        || statusText.contains("source directory is missing", Qt::CaseInsensitive)
		        || statusText.contains("nvapi.h", Qt::CaseInsensitive)))
		{
			return "Run Clean Source Dependency Cache, then retry Sync Code. The launcher will repopulate stale dependency checkouts "
			       "automatically.";
		}
		if (operationId.startsWith("workspace.build") || statusText.contains("cmake", Qt::CaseInsensitive)
		    || statusText.contains("MSBuild", Qt::CaseInsensitive) || statusText.contains("tool", Qt::CaseInsensitive))
		{
			return "Open Sync, review the missing machine prerequisites, then retry this workflow.";
		}
		if (statusText.contains("Rider", Qt::CaseInsensitive))
		{
			return "Install Rider or switch the IDE selector back to Visual Studio, then retry.";
		}
		if (statusText.contains("disabled in this workspace configuration", Qt::CaseInsensitive)
		    || statusText.contains("No cook features are enabled", Qt::CaseInsensitive))
		{
			return "This workflow is disabled by the current workspace features. Enable the required feature, then sync and build again.";
		}
		if (statusText.contains("dxcompiler.dll", Qt::CaseInsensitive) || statusText.contains("slang-compiler.dll", Qt::CaseInsensitive)
		    || statusText.contains("runtime dll is missing", Qt::CaseInsensitive)
		    || statusText.contains("runtime dependency is missing", Qt::CaseInsensitive)
		    || statusText.contains("runtime support bundle is incomplete", Qt::CaseInsensitive))
		{
			return "Run Build > Build Cooking Tools after Sync shows the Vulkan SDK and shader compiler bundle as ready, then retry this "
			       "workflow.";
		}
		if (statusText.contains("shader package", Qt::CaseInsensitive) || statusText.contains("shader", Qt::CaseInsensitive))
		{
			return "Run Cook > Cook Shaders, then retry this workflow.";
		}
		if (operationId.startsWith("cook."))
		{
			return "Review the output below. If tools or cooked inputs are missing, run Build Cooking Tools before retrying.";
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

			QString message = customCleanRequested ? "Generated outputs to clean:\n\n" + scopeNames.join("\n\n")
			                                       : "Clean scopes:\n" + scopeNames.join('\n');
			message += customCleanRequested ? "\n\nThis removes only the generated outputs mapped to the selected action. Continue?"
			                                : "\n\nThis removes generated files for the selected scope. Continue?";
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
		    GetLauncherArtifactDirectory(m_repositoryRoot, m_settings.EditorProfile().toStdString())
		    / std::filesystem::path(QCoreApplication::applicationFilePath().toStdString()).filename();
		const QString executablePath = QString::fromStdString(relaunchedExecutablePath.string());
		const bool started = QProcess::startDetached(executablePath, {});
		if (!started)
		{
			QMessageBox::warning(this, "Restart Failed", "The rebuilt launcher is ready, but the restart command could not be started.");
			return;
		}

		QCoreApplication::quit();
	}

	QString LauncherMainWindow::CreateRunId()
	{
		return QStringLiteral("run-%1").arg(++m_nextRunIndex, 4, 10, QChar('0'));
	}

	QString LauncherMainWindow::StartOperation(LauncherOperationRequest request, const QString& title)
	{
		if (request.RunId.isEmpty())
		{
			request.RunId = CreateRunId();
		}
		const QString runId = request.RunId;
		RegisterRun(request.RunId, title);
		TrackSourceDependencyRun(request, runId);
		m_backend.RunOperation(std::move(request));
		return runId;
	}
}
