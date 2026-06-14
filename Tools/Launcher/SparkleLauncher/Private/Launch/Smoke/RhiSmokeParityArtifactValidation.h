#pragma once

#include "SparkleLauncher/LaunchOperations.h"

#include <string>

namespace SparkleLauncher
{
	bool ValidateRhiSmokeRayTracingParityArtifacts(const LaunchOperationPlan& plan, std::string& outFailureSummary);
}
