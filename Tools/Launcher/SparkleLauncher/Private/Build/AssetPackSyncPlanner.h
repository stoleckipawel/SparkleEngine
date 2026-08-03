#pragma once

#include <string>
#include <span>
#include <vector>

struct ProjectLevelCatalog;

namespace SparkleLauncher
{
	std::vector<std::string> BuildAssetPackSyncPlan(
	    const ProjectLevelCatalog& catalog,
	    std::span<const std::string> requestedLevelIds = {});
}
