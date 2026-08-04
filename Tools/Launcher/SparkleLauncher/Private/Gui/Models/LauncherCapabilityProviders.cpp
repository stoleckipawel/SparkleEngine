#include "LauncherCapabilityProviders.h"

namespace SparkleLauncher
{
	bool LauncherCapabilityContext::IsRuntimeProduct() const
	{
		return Request.LaunchTarget == "runtime";
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
