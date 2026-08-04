#include "LauncherCapabilityProviders.h"

#include "SparkleLauncher/LaunchOperations.h"

#include <optional>

namespace SparkleLauncher
{
	bool LauncherCapabilityContext::IsLaunchGoal() const
	{
		return FindLaunchOperationDefinition(Request.OperationId.toStdString()).has_value();
	}

	bool LauncherCapabilityContext::IsRuntimeProduct() const
	{
		const std::optional<LaunchOperationDefinition> definition = FindLaunchOperationDefinition(Request.OperationId.toStdString());
		return definition.has_value() && definition->Product == LaunchProduct::Runtime;
	}

	QString LauncherCapabilityContext::ProductBuildOperationId() const
	{
		return IsRuntimeProduct() ? QStringLiteral("workspace.build.runtime") : QStringLiteral("workspace.build.editor");
	}

	std::string LauncherCapabilityContext::ProductCapabilityId() const
	{
		return IsRuntimeProduct() ? "product.runtime" : "product.editor";
	}

	std::string LauncherCapabilityContext::ProjectCapabilityId() const
	{
		return IsRuntimeProduct() ? "content.project.runtime" : "content.project.editor";
	}

	std::string BuildCapabilityReadinessSummary(const std::vector<std::string>& messages)
	{
		std::string summary;
		for (const std::string& message : messages)
		{
			if (message.empty())
			{
				continue;
			}
			if (!summary.empty())
			{
				summary += '\n';
			}
			summary += message;
		}
		return summary;
	}
}
