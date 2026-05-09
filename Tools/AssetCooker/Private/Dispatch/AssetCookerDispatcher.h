#pragma once

#include "../Diagnostics/AssetCookerDiagnostics.h"
#include "../Planning/ProjectCookPlan.h"

#include <filesystem>

class AssetCookerDispatcher final
{
public:
	static bool ValidateCapabilities(
	    const AssetCookerProjectCookPlan& plan,
	    AssetCookerDiagnostics& diagnostics);
	static bool DispatchPlan(
	    const AssetCookerProjectCookPlan& plan,
	    AssetCookerDiagnostics& diagnostics,
	    std::vector<AssetCookerOutputRecord>& outOutputs);
};
