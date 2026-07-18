#include "PCH.h"

#include "Cooking/ShaderCookPlanExecutor.h"

#include "Cooking/ShaderCookDiagnostics.h"
#include "Cooking/ShaderCookNodeExecutor.h"
#include "Cooking/ShaderCookPlanBuilder.h"
#include "Cooking/ShaderCookSettings.h"
#include "TaskExecutor.h"

#include <algorithm>
#include <thread>

bool ShaderCookPlanExecutor::Execute(
    const ShaderPackageCookSettings& settings,
    const ShaderCookPipelinePlan& plan,
    const std::filesystem::path& cacheDirectory,
    std::vector<ShaderCookNodeResult>& outNodeResults,
    std::string& outErrorMessage)
{
	const std::uint32_t hardwareThreads = std::max(1u, std::thread::hardware_concurrency());
	const std::uint32_t compileWorkers = std::clamp(settings.maximumParallelCompiles, 1u, std::min(hardwareThreads, 8u));
	TaskExecutor executor(
	    TaskExecutorConfig{
	        .FrameCriticalWorkerCount = 1,
	        .BackgroundWorkerCount = compileWorkers,
	        .MaximumTasksPerExecution = static_cast<std::uint32_t>(std::max<std::size_t>(plan.nodes.size(), 1)),
	        .MaximumEdgesPerExecution = 1,
	        .MaximumActiveExecutions = 1});
	outNodeResults.clear();
	outNodeResults.resize(plan.nodes.size());
	TaskGraphBuilder graph(
	    TaskGraphLimits{.MaximumTasks = static_cast<std::uint32_t>(std::max<std::size_t>(plan.nodes.size(), 1)), .MaximumEdges = 1});
	for (std::uint32_t nodeIndex = 0; nodeIndex < plan.nodes.size(); ++nodeIndex)
	{
		graph.Add(
		    TaskDesc{TaskName("Compile shader cook node"), TaskLane::Background},
		    [&, nodeIndex](TaskExecutionContext& context)
		    {
			    if (context.IsCancellationRequested())
				    return TaskResult::Cancelled("Shader cook was cancelled.");
			    ShaderCookNodeExecutor::Execute(settings, plan.nodes[nodeIndex], cacheDirectory, outNodeResults[nodeIndex]);
			    return outNodeResults[nodeIndex].Succeeded ? TaskResult::Success()
			                                               : TaskResult::Failure(outNodeResults[nodeIndex].Diagnostic);
		    });
	}
	TaskExecutionContext context;
	const TaskExecution execution = executor.Submit(graph.Compile(), context);
	if (execution.GetStatus() == TaskExecutionStatus::Succeeded)
	{
		outErrorMessage.clear();
		return true;
	}

	for (std::size_t index = 0; index < outNodeResults.size(); ++index)
	{
		if (!outNodeResults[index].Succeeded && !outNodeResults[index].Diagnostic.empty())
		{
			outErrorMessage = ShaderCookDiagnostics::FormatNodeContext(
			                      plan.nodes[index],
			                      plan.nodes[index].backendName,
			                      plan.nodes[index].compileOptions.Target) +
			                  " - " + outNodeResults[index].Diagnostic;
			return false;
		}
	}
	outErrorMessage = "Shader cook execution did not settle successfully.";
	return false;
}
