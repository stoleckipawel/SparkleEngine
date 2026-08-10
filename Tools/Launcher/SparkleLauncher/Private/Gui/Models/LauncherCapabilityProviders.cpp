#include "LauncherCapabilityProviders.h"

namespace SparkleLauncher
{
	bool LauncherCapabilityContext::IsLevelRunGoal() const
	{
		return Request.OperationId == QStringLiteral("levels.run");
	}

	bool LauncherCapabilityContext::UsesEditorProduct() const
	{
		return Request.RunMode != QStringLiteral("game");
	}

	QString LauncherCapabilityContext::ProductBuildOperationId() const
	{
		return UsesEditorProduct() ? QStringLiteral("workspace.build.editor") : QStringLiteral("workspace.build.runtime");
	}

	std::string LauncherCapabilityContext::ProductCapabilityId() const
	{
		return UsesEditorProduct() ? "product.editor" : "product.runtime";
	}

	std::string LauncherCapabilityContext::ProjectCapabilityId() const
	{
		return UsesEditorProduct() ? "content.project.editor" : "content.project.runtime";
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
