#pragma once

#include "SparkleLauncher/LaunchOperations.h"

#include <string>

namespace SparkleLauncher
{
	bool ValidateRhiSmokePtlasBenchmarkArtifacts(const LaunchOperationPlan& plan, std::string& outFailureSummary);
}
