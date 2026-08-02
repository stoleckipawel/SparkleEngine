#pragma once

#include "SparkleLauncher/BuildWorkspaceOperations.h"

#include <string>
#include <span>
#include <vector>

struct ProjectLevelCatalog;

namespace SparkleLauncher
{
	std::vector<std::string> BuildAssetPackSyncPlan(
	    const ProjectLevelCatalog& catalog,
	    BuildWorkspaceOperationKind operationKind,
	    std::span<const std::string> requestedLevelIds = {});
}
