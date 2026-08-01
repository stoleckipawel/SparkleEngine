#include "PCH.h"

#include "World/Systems/GameSystemGraph.h"

#include "World/Systems/CompiledGameSystemGraphData.h"
#include "World/Systems/Execution/GameSystemExecution.h"
#include "Tasks/Public/TaskExecution.h"
#include "Tasks/Public/TaskExecutionContext.h"
#include "Tasks/Public/TaskExecutor.h"

#include <string>

namespace ECS
{
	bool CompiledGameSystemGraph::Execute(
	    TaskExecutor& executor,
	    std::span<const GameSystemExecutionBinding> bindings,
	    GameSystemGraphError& error) const
	{
		if (!IsValid() || bindings.size() != m_data->Systems.size())
		{
			error = {GameSystemGraphErrorCode::BindingMismatch, "Game-system execution bindings do not match the compiled topology."};
			return false;
		}
		for (std::size_t index = 0; index < bindings.size(); ++index)
		{
			if (bindings[index].Id != m_data->Systems[index].Id)
			{
				error = {GameSystemGraphErrorCode::BindingMismatch, "Game-system execution binding identity is stale or reordered."};
				return false;
			}
		}

		GameSystemExecutionData execution{bindings};
		TaskExecutionContext context(execution);
		TaskExecution result = executor.Submit(m_data->Tasks, context);
		if (!result.IsValid() || result.GetStatus() != TaskExecutionStatus::Succeeded)
		{
			error = {
			    GameSystemGraphErrorCode::ExecutionFailed,
			    result.IsValid() ? std::string(result.GetResult().GetMessage()) : "SparkleTasks rejected the game-system execution."};
			return false;
		}

		error = {};
		return true;
	}
}
