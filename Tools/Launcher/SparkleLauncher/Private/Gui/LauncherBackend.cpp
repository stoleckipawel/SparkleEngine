#include "LauncherBackend.h"

#include "SparkleLauncher/BuildWorkspaceOperations.h"
#include "SparkleLauncher/CookOperations.h"
#include "SparkleLauncher/LaunchOperations.h"
#include "SparkleLauncher/MaintenanceOperations.h"
#include <QtCore/QProcess>
#include <QtCore/QRegularExpression>
#include <QtCore/QThread>

#include <memory>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace SparkleLauncher
{
	static std::vector<std::string> SplitOptionList(const QString& text)
	{
		std::vector<std::string> values;
		for (const QString& part : text.split(QRegularExpression("[,;\\n]"), Qt::SkipEmptyParts))
		{
			const QString trimmed = part.trimmed();
			if (!trimmed.isEmpty())
			{
				values.push_back(trimmed.toStdString());
			}
		}
		return values;
	}

	static std::vector<std::string> SplitCommandLineArguments(const QString& text)
	{
		std::vector<std::string> values;
		for (const QString& part : QProcess::splitCommand(text))
		{
			if (!part.isEmpty())
			{
				values.push_back(part.toStdString());
			}
		}
		return values;
	}

	static FormatMode ToFormatMode(const QString& text)
	{
		return text == "apply" ? FormatMode::Apply : FormatMode::Check;
	}

	static CleanScope ToCleanScope(const QString& text)
	{
		if (text == "all-cooked")
		{
			return CleanScope::AllCookedOutputs;
		}
		if (text == "build-tree")
		{
			return CleanScope::BuildTree;
		}
		if (text == "shader-cache")
		{
			return CleanScope::ShaderCache;
		}
		if (text == "deps")
		{
			return CleanScope::ThirdPartyDependencyCache;
		}
		if (text == "logs")
		{
			return CleanScope::Logs;
		}
		if (text == "pristine")
		{
			return CleanScope::PristineGeneratedWorkspace;
		}
		return CleanScope::SelectedProjectCookedOutputs;
	}

	static std::vector<CleanScope> SplitCleanScopes(const QString& text)
	{
		std::vector<CleanScope> scopes;
		for (const QString& part : text.split(QRegularExpression("[,;\\n]"), Qt::SkipEmptyParts))
		{
			const QString trimmed = part.trimmed();
			if (!trimmed.isEmpty())
			{
				scopes.push_back(ToCleanScope(trimmed));
			}
		}
		if (scopes.empty())
		{
			scopes.push_back(CleanScope::SelectedProjectCookedOutputs);
		}
		return scopes;
	}

	static void AppendLine(std::ostringstream& output, std::string_view text)
	{
		output << text << '\n';
	}

	static void AppendPlanDetails(
	    std::ostringstream& output,
	    const OperationRecord& operation,
	    bool canRun,
	    const std::vector<std::string>& readinessMessages,
	    const std::vector<std::string>& plannedEffects)
	{
		AppendLine(output, operation.DisplayName + std::string(canRun ? " [Ready]" : " [Blocked]"));
		AppendLine(output, "Operation: " + operation.Id);
		if (!operation.LogPath.empty())
		{
			AppendLine(output, "Log: " + operation.LogPath.string());
		}
		if (operation.RequiresConfirmation)
		{
			if (operation.DestructiveScope == OperationDestructiveScope::None)
			{
				AppendLine(output, "Confirmation required: selected clean scopes");
			}
			else
			{
				AppendLine(output, "Confirmation required: " + ToString(operation.DestructiveScope));
			}
		}
		for (const std::string& message : readinessMessages)
		{
			AppendLine(output, "Readiness: " + message);
		}
		for (const std::string& effect : plannedEffects)
		{
			AppendLine(output, "Effect: " + effect);
		}
		if (!operation.DryRunText.empty())
		{
			AppendLine(output, "");
			AppendLine(output, operation.DryRunText);
		}
	}

	static BuildWorkspaceOperationRequest MakeBuildRequest(const LauncherOperationRequest& request)
	{
		BuildWorkspaceOperationRequest buildRequest;
		WorkspaceIde workspaceIde = WorkspaceIde::VisualStudio;
		TryParseWorkspaceIde(request.WorkspaceIde.toStdString(), workspaceIde);
		buildRequest.RepositoryRoot = request.RepositoryRoot;
		buildRequest.ProjectId = request.ProjectId.toStdString();
		buildRequest.EditorProfile = request.EditorProfile.toStdString();
		buildRequest.RuntimeProfile = request.RuntimeProfile.toStdString();
		buildRequest.PreferredIde = workspaceIde;
		buildRequest.SelectedTargets = SplitOptionList(request.SelectedTargets);
		buildRequest.ForceConfigure = request.ForceConfigure;
		return buildRequest;
	}

	static CookOperationRequest MakeCookRequest(const LauncherOperationRequest& request)
	{
		CookOperationRequest cookRequest;
		cookRequest.RepositoryRoot = request.RepositoryRoot;
		cookRequest.ProjectId = request.ProjectId.toStdString();
		cookRequest.RuntimeProfile = request.RuntimeProfile.toStdString();
		cookRequest.Mode = request.ForceRecook ? CookMode::Force : CookMode::Incremental;
		cookRequest.ForceRecookConfirmed = request.ConfirmForceRecook;
		cookRequest.ShaderPackages = SplitOptionList(request.ShaderPackages);
		return cookRequest;
	}

	static MaintenanceOperationRequest MakeMaintenanceRequest(const LauncherOperationRequest& request)
	{
		MaintenanceOperationRequest maintenanceRequest;
		maintenanceRequest.RepositoryRoot = request.RepositoryRoot;
		maintenanceRequest.ProjectId = request.ProjectId.toStdString();
		maintenanceRequest.EditorProfile = request.EditorProfile.toStdString();
		maintenanceRequest.RequestedFormatMode = ToFormatMode(request.FormatMode);
		maintenanceRequest.RequestedCleanScope = ToCleanScope(request.CleanScope);
		maintenanceRequest.RequestedCleanScopes = SplitCleanScopes(request.CleanScope);
		maintenanceRequest.DestructiveActionConfirmed = request.ConfirmClean;
		return maintenanceRequest;
	}

	static LaunchOperationRequest MakeLaunchRequest(const LauncherOperationRequest& request)
	{
		LaunchOperationRequest launchRequest;
		launchRequest.RepositoryRoot = request.RepositoryRoot;
		launchRequest.ProjectId = request.ProjectId.toStdString();
		launchRequest.EditorProfile = request.EditorProfile.toStdString();
		launchRequest.RuntimeProfile = request.RuntimeProfile.toStdString();
		launchRequest.GraphicsBackend = request.LaunchBackend.toStdString();
		launchRequest.VSync = request.LaunchVSync.toStdString();
		launchRequest.PreferHighPerformanceAdapter = request.LaunchHighPerformanceAdapter.toStdString();
		launchRequest.MeshAutoBatching = request.LaunchMeshAutoBatching.toStdString();
		launchRequest.CustomArguments = SplitCommandLineArguments(request.LaunchCommandLineArguments);
		launchRequest.CustomCVars = SplitOptionList(request.LaunchCVars);
		launchRequest.SmokeBackend = request.SmokeBackend.toStdString();
		launchRequest.SmokeFrameLimit = request.SmokeFrameLimit.toStdString();
		launchRequest.SmokeTrace = request.SmokeTrace;
		launchRequest.SmokeSkipLevelSwitching = request.SmokeSkipLevelSwitching;
		return launchRequest;
	}

	static QString ToQtString(const std::ostringstream& output)
	{
		return QString::fromStdString(output.str());
	}

	static QString FormatOperationCompletion(const OperationRecord& operation)
	{
		QString text = QString::fromStdString(ToString(operation.Status));
		if (operation.ExitCode.has_value())
		{
			text += QStringLiteral(" (exit code %1)").arg(*operation.ExitCode);
		}
		if (!operation.FailureSummary.empty())
		{
			text += QString::fromStdString(" - " + operation.FailureSummary);
		}
		return text;
	}

	LauncherBackend::LauncherBackend(QObject* parent)
	    : LauncherBackend({}, parent)
	{
	}

	LauncherBackend::LauncherBackend(ProcessRunnerFactory processRunnerFactory, QObject* parent)
	    : QObject(parent)
	    , m_processRunnerFactory(std::move(processRunnerFactory))
	{
		if (!m_processRunnerFactory)
		{
			m_processRunnerFactory = []() { return std::make_unique<NativeProcessRunner>(); };
		}
		PopulateOperationCatalog();
	}

	const QVector<LauncherOperationDescriptor>& LauncherBackend::Operations() const
	{
		return m_operations;
	}

	void LauncherBackend::RequestOperationPreview(const LauncherOperationRequest& request)
	{
		const LauncherOperationDescriptor* operation = FindOperation(request.OperationId);
		if (operation == nullptr)
		{
			emit OperationPreviewFailed(request.OperationId, "Unknown launcher operation.");
			return;
		}

		std::ostringstream output;
		bool canRun = false;
		const std::string operationId = request.OperationId.toStdString();
		switch (operation->Category)
		{
		case LauncherOperationCategory::Workspace:
		{
			const BuildWorkspaceOperationPlan plan = PlanBuildWorkspaceOperation(operationId, MakeBuildRequest(request));
			AppendPlanDetails(output, plan.Operation, plan.CanRun, plan.ReadinessMessages, plan.PlannedEffects);
			canRun = plan.CanRun;
			break;
		}
		case LauncherOperationCategory::Cooking:
		{
			const CookOperationPlan plan = PlanCookOperation(operationId, MakeCookRequest(request));
			AppendPlanDetails(output, plan.Operation, plan.CanRun, plan.ReadinessMessages, plan.PlannedEffects);
			canRun = plan.CanRun;
			break;
		}
		case LauncherOperationCategory::Maintenance:
		{
			const MaintenanceOperationPlan plan = PlanMaintenanceOperation(operationId, MakeMaintenanceRequest(request));
			AppendPlanDetails(output, plan.Operation, plan.CanRun, plan.ReadinessMessages, plan.PlannedEffects);
			canRun = plan.CanRun;
			break;
		}
		case LauncherOperationCategory::Launch:
		{
			const LaunchOperationPlan plan = PlanLaunchOperation(operationId, MakeLaunchRequest(request));
			AppendPlanDetails(output, plan.Operation, plan.CanRun, plan.ReadinessMessages, plan.PlannedEffects);
			canRun = plan.CanRun;
			break;
		}
		}

		emit OperationPreviewReady(operation->Id, operation->DisplayName, ToQtString(output), canRun);
	}

	void LauncherBackend::RunOperation(LauncherOperationRequest request)
	{
		const LauncherOperationDescriptor* operation = FindOperation(request.OperationId);
		if (operation == nullptr)
		{
			emit OperationPreviewFailed(request.OperationId, "Unknown launcher operation.");
			return;
		}

		const QString runId = request.RunId.isEmpty() ? operation->Id : request.RunId;
		emit OperationStarted(runId, operation->Id, operation->DisplayName);
		const LauncherOperationCategory category = operation->Category;
		const QString title = operation->DisplayName;
		const QString operationIdText = operation->Id;
		ProcessRunnerFactory processRunnerFactory = m_processRunnerFactory;

		QThread* workerThread = QThread::create([this, category, title, runId, operationIdText, processRunnerFactory, request = std::move(request)]() {
			const std::string operationId = request.OperationId.toStdString();
			std::unique_ptr<IProcessRunner> processRunner = processRunnerFactory();
			if (processRunner == nullptr)
			{
				OperationRecord failedRecord = MakeOperationRecord(operationId, title.toStdString());
				failedRecord.Status = OperationStatus::Failed;
				failedRecord.FailureSummary = "No process runner is available for this launcher operation.";
				emit OperationFinished(runId, operationIdText, title, FormatOperationCompletion(failedRecord), -1);
				return;
			}
			const ProcessOutputCallback outputCallback = [this, runId, operationIdText](std::string_view output) {
				emit OperationOutputReceived(runId, operationIdText, QString::fromStdString(std::string(output)));
			};

			OperationRecord record = MakeOperationRecord(operationId, title.toStdString());
			switch (category)
			{
			case LauncherOperationCategory::Workspace:
			{
				BuildWorkspaceOperationPlan plan = PlanBuildWorkspaceOperation(operationId, MakeBuildRequest(request));
				if (!plan.CanRun)
				{
					record = plan.Operation;
					record.Status = OperationStatus::Skipped;
					record.FailureSummary = "Operation readiness failed.";
					break;
				}
				record = RunBuildWorkspaceOperationPlan(std::move(plan), *processRunner, outputCallback);
				break;
			}
			case LauncherOperationCategory::Cooking:
			{
				CookOperationPlan plan = PlanCookOperation(operationId, MakeCookRequest(request));
				if (!plan.CanRun)
				{
					record = plan.Operation;
					record.Status = OperationStatus::Skipped;
					record.FailureSummary = "Operation readiness failed.";
					break;
				}
				record = RunCookOperationPlan(std::move(plan), *processRunner, outputCallback);
				break;
			}
			case LauncherOperationCategory::Maintenance:
			{
				MaintenanceOperationPlan plan = PlanMaintenanceOperation(operationId, MakeMaintenanceRequest(request));
				if (!plan.CanRun)
				{
					record = plan.Operation;
					record.Status = OperationStatus::Skipped;
					record.FailureSummary = "Operation readiness failed.";
					break;
				}
				record = RunMaintenanceOperationPlan(std::move(plan), *processRunner, outputCallback);
				break;
			}
			case LauncherOperationCategory::Launch:
			{
				LaunchOperationPlan plan = PlanLaunchOperation(operationId, MakeLaunchRequest(request));
				if (!plan.CanRun)
				{
					record = plan.Operation;
					record.Status = OperationStatus::Skipped;
					record.FailureSummary = "Operation readiness failed.";
					break;
				}
				record = RunLaunchOperationPlan(std::move(plan), *processRunner, outputCallback);
				break;
			}
			}

			emit OperationFinished(runId, operationIdText, title, FormatOperationCompletion(record), record.ExitCode.value_or(-1));
		});
		connect(workerThread, &QThread::finished, workerThread, &QObject::deleteLater);
		workerThread->start();
	}

	void LauncherBackend::PopulateOperationCatalog()
	{
		m_operations.clear();

		for (const BuildWorkspaceOperationDefinition& definition : GetBuildWorkspaceOperationDefinitions())
		{
			m_operations.push_back(
			    {QString::fromStdString(definition.Id),
			     QString::fromStdString(definition.Group),
			     QString::fromStdString(definition.DisplayName),
			     QString::fromStdString(definition.Description),
			     LauncherOperationCategory::Workspace});
		}

		for (const CookOperationDefinition& definition : GetCookOperationDefinitions())
		{
			m_operations.push_back(
			    {QString::fromStdString(definition.Id),
			     QString::fromStdString(definition.Group),
			     QString::fromStdString(definition.DisplayName),
			     QString::fromStdString(definition.Description),
			     LauncherOperationCategory::Cooking});
		}

		for (const MaintenanceOperationDefinition& definition : GetMaintenanceOperationDefinitions())
		{
			m_operations.push_back(
			    {QString::fromStdString(definition.Id),
			     QString::fromStdString(definition.Group),
			     QString::fromStdString(definition.DisplayName),
			     QString::fromStdString(definition.Description),
			     LauncherOperationCategory::Maintenance});
		}

		for (const LaunchOperationDefinition& definition : GetLaunchOperationDefinitions())
		{
			m_operations.push_back(
			    {QString::fromStdString(definition.Id),
			     QString::fromStdString(definition.Group),
			     QString::fromStdString(definition.DisplayName),
			     QString::fromStdString(definition.Description),
			     LauncherOperationCategory::Launch});
		}
	}

	const LauncherOperationDescriptor* LauncherBackend::FindOperation(const QString& operationId) const
	{
		for (const LauncherOperationDescriptor& operation : m_operations)
		{
			if (operation.Id == operationId)
			{
				return &operation;
			}
		}

		return nullptr;
	}
}
