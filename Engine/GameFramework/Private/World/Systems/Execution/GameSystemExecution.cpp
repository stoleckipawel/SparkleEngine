#include "PCH.h"

#include "World/Systems/Execution/GameSystemExecution.h"

#include "Tasks/Public/TaskExecutionContext.h"

#include <algorithm>

namespace ECS
{
	std::uint32_t GameSystemExecution::ResolvePartitionCount(std::uint32_t itemCount, const ParallelForPolicy& policy) noexcept
	{
		if (itemCount == 0)
		{
			return 0;
		}
		if (itemCount <= policy.SerialThreshold || itemCount <= policy.GrainSize)
		{
			return 1;
		}

		const std::uint32_t grainPartitions =
		    static_cast<std::uint32_t>((static_cast<std::uint64_t>(itemCount) + policy.GrainSize - 1u) / policy.GrainSize);
		return (std::min) (policy.MaximumPartitions, grainPartitions);
	}

	TaskResult GameSystemExecution::ExecutePartition(
	    std::uint32_t systemIndex,
	    std::uint32_t partitionIndex,
	    const ParallelForPolicy& policy,
	    TaskExecutionContext& taskContext)
	{
		if (taskContext.IsCancellationRequested())
		{
			return TaskResult::Cancelled("Game-system execution cancelled at the owner boundary.");
		}

		GameSystemExecutionData* execution = taskContext.TryGet<GameSystemExecutionData>();
		if (execution == nullptr || systemIndex >= execution->Bindings.size())
		{
			return TaskResult::Failure("Game-system execution binding is unavailable.");
		}

		const GameSystemExecutionBinding& binding = execution->Bindings[systemIndex];
		if (!binding.GetItemCount || !binding.ExecuteRange)
		{
			return TaskResult::Failure("Game-system execution binding is incomplete.");
		}

		const std::uint32_t itemCount = binding.GetItemCount();
		const std::uint32_t partitionCount = ResolvePartitionCount(itemCount, policy);
		if (partitionIndex >= partitionCount)
		{
			return TaskResult::Success();
		}

		const std::uint32_t partitionSize =
		    static_cast<std::uint32_t>((static_cast<std::uint64_t>(itemCount) + partitionCount - 1u) / partitionCount);
		const std::uint32_t begin = partitionIndex * partitionSize;
		const std::uint32_t end = (std::min) (itemCount, begin + partitionSize);
		return begin == end || binding.ExecuteRange(begin, end)
		           ? TaskResult::Success()
		           : TaskResult::Failure("Game-system range rejected its declared access or target range.");
	}
}
