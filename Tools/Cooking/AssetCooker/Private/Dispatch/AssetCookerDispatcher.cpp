#include "AssetCookerDispatcher.h"

#include "AssetCookerStageExecutor.h"
#include "Core/Public/FileSystemUtils.h"
#include "ToolConsole.h"

#include <iostream>
#include <string>

bool AssetCookerDispatcher::ValidateCapabilities(
    const AssetCookerProjectCookPlan& plan, AssetCookerDiagnostics& diagnostics)
{
	return AssetCookerStageExecutor::ValidateCapabilities(plan, diagnostics);
}

bool AssetCookerDispatcher::DispatchPlan(
    const AssetCookerProjectCookPlan& plan,
    AssetCookerDiagnostics& diagnostics,
	std::vector<AssetCookerOutputRecord>& outOutputs)
{
	Filesystem::ConfigureProjectRoot(plan.projectRoot);

	for (std::size_t stepIndex = 0; stepIndex < plan.steps.size(); ++stepIndex)
	{
		const AssetCookerPlanStep step = plan.steps[stepIndex];
		const char* stepName = AssetCookerStageExecutor::GetStepName(step);
		ToolConsole::Progress(std::cout, "Cooking", "stage", stepIndex + 1u, plan.steps.size(), stepName);

		const bool succeeded = AssetCookerStageExecutor::Execute(step, plan, diagnostics, outOutputs);
		if (!succeeded)
		{
			return false;
		}
	}
	return true;
}
