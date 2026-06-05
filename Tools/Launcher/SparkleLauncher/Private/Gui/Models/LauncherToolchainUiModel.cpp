#include "LauncherToolchainUiModel.h"

#include <QtCore/QStringList>

namespace SparkleLauncher
{
	QString ToolchainStatusState(ToolchainItemState state, bool required)
	{
		switch (state)
		{
		case ToolchainItemState::Found:
			return "ok";
		case ToolchainItemState::Warning:
			return required ? "warning" : "neutral";
		case ToolchainItemState::Missing:
			return required ? "bad" : "neutral";
		}
		return "neutral";
	}

	QString ToolchainStatusText(ToolchainItemState state, bool required)
	{
		switch (state)
		{
		case ToolchainItemState::Found:
			return "Ready";
		case ToolchainItemState::Warning:
			return required ? "Warning" : "Optional";
		case ToolchainItemState::Missing:
			return required ? "Missing" : "Optional";
		}
		return "Unknown";
	}

	QString BuildGeneratorSummary(const BuildToolchainStatus& toolchain)
	{
		return QStringLiteral("Generator: %1 | Platform: %2%3%4")
		    .arg(QString::fromStdString(toolchain.Generator))
		    .arg(QString::fromStdString(toolchain.Platform))
		    .arg(toolchain.Toolset.empty() ? QString() : QStringLiteral(" | Toolset: %1").arg(QString::fromStdString(toolchain.Toolset)))
		    .arg(toolchain.QtRootPath.empty() ? QString() : QStringLiteral(" | Qt: %1").arg(QString::fromStdString(toolchain.QtRootPath.string())));
	}

	QString RequiredToolProblemSummary(const BuildToolchainStatus& toolchain)
	{
		QStringList problems;
		for (const ToolchainItemStatus& item : toolchain.Items)
		{
			if (!item.Required || item.State == ToolchainItemState::Found)
			{
				continue;
			}
			problems.push_back(QString::fromStdString(item.DisplayName));
		}

		return problems.isEmpty() ? QString() : "Missing or blocked: " + problems.join(", ");
	}
}
