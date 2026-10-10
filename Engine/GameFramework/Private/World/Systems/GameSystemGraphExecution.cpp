#include "PCH.h"
#include "World/Systems/GameSystemGraph.h"
#include "World/Systems/GameWorldSystems.h"
#include "World/Systems/CompiledGameSystemGraphData.h"
#include "Tasks/Public/TaskExecutionContext.h"
#include "Tasks/Public/TaskExecutor.h"

#include <string>

namespace ECS
{
	bool CompiledGameSystemGraph::Execute(TaskExecutor& executor, GameWorldSystemExecution& systems, GameSystemGraphError& error)
	{
		if (!IsValid())
		{
			error = GetError();
			return false;
		}
		// Counts are read at the host boundary after prerequisite commits, including newly dirtied transforms.
		for (auto& wave : m_data->Waves)
		{
			bool changed = !wave.Tasks.IsValid();
			for (std::size_t offset = 0; offset < wave.Systems.size(); ++offset)
			{
				const auto count = m_data->Systems[wave.Systems[offset]].GetItemCount(systems);
				changed |= count != wave.ItemCounts[offset];
				wave.ItemCounts[offset] = count;
			}
			if (changed)
			{
				TaskGraphBuilder tasks;
				for (std::size_t offset = 0; offset < wave.Systems.size(); ++offset)
				{
					const GameSystemDesc& system = m_data->Systems[wave.Systems[offset]];
					const auto count = wave.ItemCounts[offset];
					if (count == 0)
					{
						continue;
					}

					ParallelFor(
					    tasks,
					    TaskDesc{TaskName(system.Name), TaskLane::FrameCritical},
					    count,
					    system.RangePolicy,
					    [&system](std::uint32_t begin, std::uint32_t end, TaskExecutionContext& context)
					    {
						    if (context.IsCancellationRequested())
						    {
							    return TaskResult::Cancelled("Game-system execution cancelled at the owner boundary.");
						    }
						    auto* execution = context.TryGet<GameWorldSystemExecution>();
						    return execution != nullptr && system.ExecuteRange(*execution, begin, end) ? TaskResult::Success()
						                                                                               : TaskResult::Failure("Game-system range rejected its declared access or target range.");
					    });
				}
				wave.Tasks = tasks.Compile();
				if (!wave.Tasks)
				{
					error = {GameSystemGraphErrorCode::TaskGraphRejected, wave.Tasks.GetError().Message};
					return false;
				}
			}
			if (wave.Tasks.GetTaskCount() == 0)
			{
				continue;
			}
			TaskExecutionContext context(systems);
			TaskExecution result = executor.Submit(wave.Tasks, context);
			if (!result.IsValid() || result.GetStatus() != TaskExecutionStatus::Succeeded)
			{
				error = {GameSystemGraphErrorCode::ExecutionFailed, result.IsValid() ? std::string(result.GetResult().GetMessage()) : "SparkleTasks rejected the game-system execution."};
				return false;
			}
		}
		error = {};
		return true;
	}
}
