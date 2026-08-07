#include "LauncherCapabilityProviders.h"

namespace SparkleLauncher
{
	bool LauncherCapabilityContext::IsLevelRunGoal() const
	{
		return Request.OperationId == QStringLiteral("levels.run");
	}

	QString LauncherCapabilityContext::ProductBuildOperationId() const
	{
		return QStringLiteral("workspace.build.runtime");
	}

	std::string LauncherCapabilityContext::ProductCapabilityId() const
	{
		return "product.runtime";
	}

	std::string LauncherCapabilityContext::ProjectCapabilityId() const
	{
		return "content.project.runtime";
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
