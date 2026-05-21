#include "PCH.h"

#include "Cooking/ShaderCookPlanExecutor.h"

#include "Cooking/ShaderCookNodeExecutor.h"
#include "Cooking/ShaderCookProgressReporter.h"

bool ShaderCookPlanExecutor::Execute(
    const ShaderPackageCookSettings& settings,
    bool writeDebugArtifacts,
    ShaderCookPipelinePlan& plan,
    ShaderBackendPool& backendPool,
    IShaderArtifactStore& artifactStore,
    ShaderCookExecutionCounters& counters,
    std::string& outErrorMessage)
{
	ShaderCookProgressReporter::PrintPlanSummary(plan, settings);
	for (const CookNode& node : plan.nodes)
	{
		if (!ShaderCookNodeExecutor::Execute(
		        settings,
		        writeDebugArtifacts,
		        node,
		        plan,
		        backendPool,
		        artifactStore,
		        counters,
		        outErrorMessage))
		{
			return false;
		}
	}

	outErrorMessage.clear();
	return true;
}