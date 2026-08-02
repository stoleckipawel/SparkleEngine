#pragma once

#include "SparkleLauncher/BuildWorkspaceOperations.h"

#include <string>
#include <vector>

struct ProjectLevelCatalog;

namespace SparkleLauncher
{
	std::vector<std::string> BuildAssetPackSyncPlan(const ProjectLevelCatalog& catalog, BuildWorkspaceOperationKind operationKind);
}
