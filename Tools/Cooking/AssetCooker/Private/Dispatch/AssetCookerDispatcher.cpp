#include "AssetCookerDispatcher.h"

#include "AssetCookerStageExecutor.h"
#include "ToolWorkProgress.h"
#include "Core/Public/FileSystemUtils.h"
#include "ToolConsole.h"

#include <iostream>
#include <string>

bool AssetCookerDispatcher::DispatchPlan(
    const AssetCookerProjectCookPlan& plan,
    AssetCookerDiagnostics& diagnostics,
    std::vector<AssetCookerOutputRecord>& outOutputs)
{
	if (!AssetCookerStageExecutor::ValidateCapabilities(plan, diagnostics))
	{
		return false;
	}

	Filesystem::ConfigureProjectRoot(plan.projectRoot);
	ToolWorkProgressWriter progress(std::cout);

	for (std::size_t stepIndex = 0; stepIndex < plan.steps.size(); ++stepIndex)
	{
		const AssetCookerPlanStep step = plan.steps[stepIndex];
		const char* stepName = AssetCookerStageExecutor::GetStepName(step);
		const std::string phase = "Asset cook stage: " + std::string(stepName);
		progress.Report(phase, stepIndex, plan.steps.size());

		const bool succeeded = AssetCookerStageExecutor::Execute(step, plan, diagnostics, outOutputs);
		if (!succeeded)
		{
			return false;
		}
		progress.Report(phase, stepIndex + 1u, plan.steps.size());
	}
	return true;
}
