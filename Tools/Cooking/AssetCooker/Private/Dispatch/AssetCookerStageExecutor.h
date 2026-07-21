#pragma once

#include "Diagnostics/AssetCookerDiagnostics.h"
#include "Planning/ProjectCookPlan.h"

#include <vector>

namespace AssetCookerStageExecutor
{
	const char* GetStepName(AssetCookerPlanStep step) noexcept;
	bool PlanUsesStep(const AssetCookerProjectCookPlan& plan, AssetCookerPlanStep step) noexcept;
	bool ValidateCapabilities(const AssetCookerProjectCookPlan& plan, AssetCookerDiagnostics& diagnostics);
	bool Execute(
	    AssetCookerPlanStep step,
	    const AssetCookerProjectCookPlan& plan,
	    AssetCookerDiagnostics& diagnostics,
	    std::vector<AssetCookerOutputRecord>& outOutputs);
}
