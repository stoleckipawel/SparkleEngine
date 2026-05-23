#include "LauncherBackend.h"

#include "SparkleLauncher/BuildWorkspaceOperations.h"
#include "SparkleLauncher/CookOperations.h"
#include "SparkleLauncher/LaunchOperations.h"
#include "SparkleLauncher/MaintenanceOperations.h"

namespace SparkleLauncher
{
	LauncherBackend::LauncherBackend(QObject* parent)
	    : QObject(parent)
	{
		PopulateOperationCatalog();
	}

	const QVector<LauncherOperationDescriptor>& LauncherBackend::Operations() const
	{
		return m_operations;
	}

	void LauncherBackend::RequestOperationPreview(const QString& operationId)
	{
		const LauncherOperationDescriptor* operation = FindOperation(operationId);
		if (operation == nullptr)
		{
			emit OperationPreviewFailed(operationId, "Unknown launcher operation.");
			return;
		}

		QString previewText;
		previewText += "Phase 1 backend adapter is connected.\n";
		previewText += "Operation: " + operation->DisplayName + "\n";
		previewText += "Group: " + operation->Group + "\n\n";
		previewText += "Phase 2 will bind this UI request to the native SparkleLauncherCore plan/run APIs. ";
		previewText += "This scaffold intentionally does not execute commands, start processes, or shell out to the legacy console app.";

		emit OperationPreviewReady(operation->Id, operation->DisplayName, previewText);
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