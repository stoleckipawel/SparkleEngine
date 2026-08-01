#pragma once

#include "Tasks/Public/TaskTypes.h"
#include "World/Systems/GameSystemGraph.h"

#include <cstdint>
#include <span>

class TaskExecutionContext;

namespace ECS
{
	struct GameSystemExecutionData final
	{
		std::span<const GameSystemExecutionBinding> Bindings;
	};

	class GameSystemExecution final
	{
	  public:
		static TaskResult ExecutePartition(
		    std::uint32_t systemIndex,
		    std::uint32_t partitionIndex,
		    const ParallelForPolicy& policy,
		    TaskExecutionContext& taskContext);

	  private:
		static std::uint32_t ResolvePartitionCount(std::uint32_t itemCount, const ParallelForPolicy& policy) noexcept;
	};
}
