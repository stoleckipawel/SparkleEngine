#pragma once

#include "SparkleLauncher/BuildWorkspaceOperations.h"

#include <QtCore/QString>

namespace SparkleLauncher
{
	QString ToolchainStatusState(ToolchainItemState state, bool required);
	QString ToolchainStatusText(ToolchainItemState state, bool required);
	QString BuildGeneratorSummary(const BuildToolchainStatus& toolchain);
	QString RequiredToolProblemSummary(const BuildToolchainStatus& toolchain);
}
