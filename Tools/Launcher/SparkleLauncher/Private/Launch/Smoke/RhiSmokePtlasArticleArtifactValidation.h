#pragma once

#include "SparkleLauncher/LaunchOperations.h"

#include <string>

namespace SparkleLauncher
{
	bool ValidateRhiSmokePtlasArticleArtifacts(const LaunchOperationPlan& plan, std::string& outFailureSummary);
}
