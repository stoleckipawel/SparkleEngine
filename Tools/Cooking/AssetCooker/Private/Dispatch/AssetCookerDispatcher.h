#pragma once

#include "../Diagnostics/AssetCookerDiagnostics.h"
#include "../Planning/ProjectCookPlan.h"

#include <vector>
#include <filesystem>

class AssetCookerDispatcher final
{
public:
	static bool DispatchPlan(
	    const AssetCookerProjectCookPlan& plan,
	    AssetCookerDiagnostics& diagnostics,
	    std::vector<AssetCookerOutputRecord>& outOutputs);
};
