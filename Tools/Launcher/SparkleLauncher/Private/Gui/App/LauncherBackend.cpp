#include "LauncherBackend.h"

#include "SparkleLauncher/BuildWorkspaceOperations.h"
#include "SparkleLauncher/CookOperations.h"
#include "SparkleLauncher/LevelOperations.h"
#include "SparkleLauncher/LaunchOperations.h"
#include "SparkleLauncher/MaintenanceOperations.h"
#include "Operations/LauncherOperationExecution.h"
#include "Operations/LauncherOperationService.h"

#include <QtCore/QMetaObject>

#include <memory>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

namespace SparkleLauncher
{
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

	LauncherBackend::LauncherBackend(QObject* parent) :
	    LauncherBackend({}, parent)
	{
	}

	LauncherBackend::LauncherBackend(ProcessRunnerFactory processRunnerFactory, QObject* parent) :
	    QObject(parent)
	{
		if (!processRunnerFactory)
		{
			processRunnerFactory = []()
			{
				return std::make_unique<NativeProcessRunner>();
			};
		}
		m_operationService = std::make_unique<LauncherOperationService>(std::move(processRunnerFactory));
		PopulateOperationCatalog();
	}

	LauncherBackend::~LauncherBackend() = default;

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
		const std::string operationId = request.OperationId.toStdString();
		const LauncherOperationPlan plan = PlanLauncherOperation(operation->Category, operationId, request);
		const bool canRun = std::visit(
		    [&output](const auto& typedPlan)
		    {
			    AppendPlanDetails(output, typedPlan.Operation, typedPlan.CanRun, typedPlan.ReadinessMessages, typedPlan.PlannedEffects);
			    return typedPlan.CanRun;
		    },
		    plan);

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
		m_operationService->Launch(
		    category,
		    std::move(request),
		    title.toStdString(),
		    [this, runId, operationIdText](std::string_view output)
		    { QueueOperationOutput(runId, operationIdText, QString::fromUtf8(output.data(), static_cast<qsizetype>(output.size()))); },
		    [this, runId, operationIdText, title](OperationRecord record)
		    { QueueOperationFinished(runId, operationIdText, title, std::move(record)); });
	}

	bool LauncherBackend::CancelOperation(const QString& runId)
	{
		return !runId.isEmpty() && m_operationService->Cancel(runId.toStdString());
	}

	void LauncherBackend::QueueOperationOutput(QString runId, QString operationId, QString outputText)
	{
		QMetaObject::invokeMethod(
		    this,
		    [this, runId = std::move(runId), operationId = std::move(operationId), outputText = std::move(outputText)]
		    { emit OperationOutputReceived(runId, operationId, outputText); },
		    Qt::QueuedConnection);
	}

	void LauncherBackend::QueueOperationFinished(QString runId, QString operationId, QString title, OperationRecord record)
	{
		const QString status = FormatOperationCompletion(record);
		const int exitCode = record.ExitCode.value_or(-1);
		QMetaObject::invokeMethod(
		    this,
		    [this, runId = std::move(runId), operationId = std::move(operationId), title = std::move(title), status, exitCode]
		    { emit OperationFinished(runId, operationId, title, status, exitCode); },
		    Qt::QueuedConnection);
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

		for (const LevelOperationDefinition& definition : GetLevelOperationDefinitions())
		{
			m_operations.push_back(
			    {QString::fromStdString(definition.Id),
			        QString::fromStdString(definition.Group),
			        QString::fromStdString(definition.DisplayName),
			        QString::fromStdString(definition.Description),
			        LauncherOperationCategory::Levels});
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
