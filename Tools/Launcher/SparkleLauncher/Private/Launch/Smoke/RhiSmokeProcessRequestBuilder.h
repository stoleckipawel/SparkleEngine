#pragma once

#include "LaunchOperationProcessRequests.h"

namespace SparkleLauncher
{
	ProcessRequest BuildRhiSmokeBaseProcessRequest(const LaunchOperationPlan& plan);
	void AddRhiSmokeCVar(ProcessRequest& request, std::string cvar);
	void AddOrReplaceRhiSmokeEnvironment(ProcessRequest& request, std::string name, std::string value);
}
