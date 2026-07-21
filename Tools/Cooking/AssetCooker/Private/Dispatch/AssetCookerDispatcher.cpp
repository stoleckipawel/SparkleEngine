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
	ToolConsole::Summary(
	    std::cout,
	    "AssetCooker cook",
	    {ToolConsole::QuotedField("project", plan.projectName),
	     ToolConsole::QuotedField("configuration", plan.configuration),
	     ToolConsole::QuotedField("toolConfiguration", plan.toolConfiguration),
	     ToolConsole::Field("steps", std::to_string(plan.steps.size())),
	     ToolConsole::Field("scenes", std::to_string(plan.sceneEntries.size()))});

	for (std::size_t stepIndex = 0; stepIndex < plan.steps.size(); ++stepIndex)
	{
		const AssetCookerPlanStep step = plan.steps[stepIndex];
		const char* stepName = AssetCookerStageExecutor::GetStepName(step);
		ToolConsole::Progress(std::cout, "Cooking", "stage", stepIndex + 1u, plan.steps.size(), stepName);

		const bool succeeded = AssetCookerStageExecutor::Execute(step, plan, diagnostics, outOutputs);
		ToolConsole::Message(
		    std::cout,
		    ToolConsoleSeverity::Info,
		    "Stage finished",
		    {ToolConsole::QuotedField("name", stepName),
		     ToolConsole::Field("status", succeeded ? "completed" : "failed")});
		if (!succeeded)
		{
			return false;
		}
	}
	return true;
}
