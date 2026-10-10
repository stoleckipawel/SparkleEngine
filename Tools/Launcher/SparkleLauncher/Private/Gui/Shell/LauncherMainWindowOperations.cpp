#include "LauncherMainWindow.h"

#include "LauncherActivityPanel.h"
#include "LauncherActionWidgets.h"
#include "LauncherBackend.h"
#include "LauncherOperationRequestFactory.h"
#include "LauncherOperationRequestMapping.h"
#include "LauncherContentModel.h"
#include "LauncherSettings.h"
#include "LauncherWorkflowCatalog.h"

#include "Core/Public/Paths/WorkspaceOutputPaths.h"

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
			m_activityPanel->ShowMessage("Choose a workflow before running.");
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
			m_activityPanel->ShowMessage(message);
			return;
		}
		const QString cleanScopeError = m_selectedOperationId == "workspace.clean" ? CleanScopeSelectionError(m_settings.CleanScope()) : QString();
		if (!cleanScopeError.isEmpty())
		{
			m_activityPanel->ShowMessage(cleanScopeError);
			return;
		}

		if (m_selectedOperationId == "levels.sync")
		{
			SyncAllLevels();
			return;
		}

		LauncherOperationRequest request = m_selectedOperationId == "workspace.clean"
		    ? BuildScopedCleanOperationRequest(m_repositoryRoot, m_contentModel, m_settings, m_settings.CleanScope(), std::filesystem::path(QCoreApplication::applicationFilePath().toStdString()))
		    : BuildLauncherOperationRequest(m_repositoryRoot, m_contentModel, m_settings, m_selectedOperationId);

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
			const QString message = OperationNeedsContent(m_selectedOperationId) && m_contentModel.ContentId().isEmpty() ? "Repository content is unavailable for this workflow's generated outputs."
			                                                                                                             : "No generated outputs were resolved for this workflow.";

			m_activityPanel->ShowMessage(message);
			return;
		}

		if (!ConfirmRunRequest(request))
		{
			return;
		}

		StartOperation(std::move(request), "Clean " + DisplayNameForOperation(m_selectedOperationId));
	}

	QPushButton* LauncherMainWindow::CreateStatusActionButton(const QString& actionId, const QString& actionLabel, const QString& actionTitle, bool navigateInsteadOfRun)
	{
		QPushButton* button = new QPushButton(this);
		ApplyStatusActionButtonPresentation(*button, actionLabel, "warning");
		button->setAccessibleName(actionTitle);
		button->setToolTip(actionTitle + ".");
		RegisterFocusable(button);
		connect(button, &QPushButton::clicked, this, [this, actionId, actionTitle, navigateInsteadOfRun]() { TriggerActionDependencyRegenerate(actionId, actionTitle, navigateInsteadOfRun); });
		return button;
	}

	void LauncherMainWindow::TriggerActionDependencyRegenerate(const QString& actionId, const QString& actionTitle, bool navigateInsteadOfRun)
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
		QString overrideName = LauncherOperationDisplayNameOverride(operationId);
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
			return CleanScopeSelectionRequiresContent(m_settings.CleanScope());
		}

		if (operationId == "workspace.build")
		{
			return m_settings.BuildScopes().contains("editor") || m_settings.BuildScopes().contains("runtime");
		}

		return operationId == LauncherHomeOperationId() || operationId == "levels.sync" || operationId == "levels.run" || operationId.startsWith("workspace.build.") || operationId.startsWith("cook.");
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
			QMessageBox::warning(const_cast<LauncherMainWindow*>(this), "Confirmation Required", "Enable Confirm clean cook before removing cooked outputs.");
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

		const Filesystem::WorkspaceOutputPaths workspaceOutputs = Filesystem::ResolveWorkspaceOutputPaths(m_repositoryRoot);
		const Filesystem::WorkspaceTargetOutputPaths launcherOutputs = workspaceOutputs.LauncherTargetOutputs(m_settings.EditorProfile().toStdString());
		const std::filesystem::path relaunchedExecutablePath = launcherOutputs.BinaryDirectory / std::filesystem::path(QCoreApplication::applicationFilePath().toStdString()).filename();
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
		if (LauncherOperationRequestMapping::RequestsLauncherRebuild(request) && !m_pendingRestartRunIds.contains(runId))
		{
			m_pendingRestartRunIds.push_back(runId);
		}
		m_activityPanel->RegisterRun(request.RunId, title);
		TrackSourceDependencyRun(request, runId);
		m_backend.RunOperation(std::move(request));
		return runId;
	}
}
