#include "PCH.h"

#include "Cooking/ShaderCookPlanExecutor.h"

#include "Cooking/ShaderCookDiagnostics.h"
#include "Cooking/ShaderCookNodeExecutor.h"
#include "Cooking/ShaderCookPlanBuilder.h"
#include "Cooking/ShaderCookSettings.h"
#include "Core/Public/Diagnostics/Error.h"
#include "Core/Public/Diagnostics/Verify.h"
#include "TaskExecutor.h"

#include <algorithm>
#include <thread>

static const auto g_shaderCookPlanExecutorLogger = Logging::GetOrCreateLogger("ShaderCompiler.CookPlanExecutor");

std::vector<CookedStageBuild> ShaderCookPlanExecutor::Execute(
    const ShaderPackageCookSettings& settings,
    const ShaderCookPipelinePlan& plan)
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
	std::vector<CookedStageBuild> compiledStages(plan.nodes.size());
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
			    compiledStages[nodeIndex] = ShaderCookNodeExecutor::Execute(settings, plan.nodes[nodeIndex]);
			    return TaskResult::Success();
		    });
	}
	TaskExecutionContext context;
	const TaskExecution execution = executor.Submit(graph.Compile(), context);
	if (execution.GetStatus() == TaskExecutionStatus::Succeeded)
	{
		return compiledStages;
	}

	const TaskResult result = execution.GetResult();
	if (result.GetMessage().empty())
	{
		Diagnostics::Fatal(
		    g_shaderCookPlanExecutorLogger,
		    __FILE__,
		    __LINE__,
		    "Shader cook task failure has no diagnostic.");
	}
	throw Diagnostics::Error(std::string(result.GetMessage()));
}
