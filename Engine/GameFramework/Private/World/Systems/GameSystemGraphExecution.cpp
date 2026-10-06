#include "PCH.h"
#include "World/Systems/GameSystemGraph.h"
#include "World/Systems/CompiledGameSystemGraphData.h"
#include "Tasks/Public/TaskExecutionContext.h"
#include "Tasks/Public/TaskExecutor.h"

#include <string>

namespace ECS
{
	bool CompiledGameSystemGraph::Execute(TaskExecutor& executor, GameWorldSystemExecution& systems, GameSystemGraphError& error) const
	{
		if (!IsValid())
		{
			error = GetError();
			return false;
		}
		// Counts are read at the host boundary after prerequisite commits, including newly dirtied transforms.
		for (const auto& wave : m_data->Waves)
		{
			TaskGraphBuilder tasks;
			for (std::uint32_t index : wave)
			{
				const GameSystemDesc& system = m_data->Systems[index];
				const std::uint32_t count = system.GetItemCount(systems);
				if (count == 0)
					continue;
				ParallelFor(
				    tasks,
				    TaskDesc{TaskName(system.Name), TaskLane::FrameCritical},
				    count,
				    system.RangePolicy,
				    [&system, &systems](std::uint32_t begin, std::uint32_t end, TaskExecutionContext& context)
				    {
					    if (context.IsCancellationRequested())
						    return TaskResult::Cancelled("Game-system execution cancelled at the owner boundary.");
					    return system.ExecuteRange(systems, begin, end)
					        ? TaskResult::Success()
					        : TaskResult::Failure("Game-system range rejected its declared access or target range.");
				    });
			}
			const CompiledTaskGraph graph = tasks.Compile();
			if (!graph)
			{
				error = {GameSystemGraphErrorCode::TaskGraphRejected, graph.GetError().Message};
				return false;
			}
			if (graph.GetTaskCount() == 0)
				continue;
			TaskExecutionContext context;
			TaskExecution result = executor.Submit(graph, context);
			if (!result.IsValid() || result.GetStatus() != TaskExecutionStatus::Succeeded)
			{
				error = {
				    GameSystemGraphErrorCode::ExecutionFailed,
				    result.IsValid() ? std::string(result.GetResult().GetMessage()) : "SparkleTasks rejected the game-system execution."};
				return false;
			}
		}
		error = {};
		return true;
	}
}
