#pragma once

#include "SparkleLauncher/CookOperations.h"

#include <filesystem>
#include <string>
#include <vector>

namespace SparkleLauncher
{
	struct CookOperationProcessStep
	{
		std::string Id;
		std::string DisplayName;
		ProcessRequest Request;
		std::filesystem::path DestructivePath;
		bool HasProcessRequest = true;
		bool DeletesCookedOutputs = false;
	};

	std::vector<CookOperationProcessStep> BuildCookProcessStepsForPlan(const CookOperationPlan& plan);
}
