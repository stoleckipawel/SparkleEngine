#pragma once

#include "TaskExecutionState.h"

#include <cstdint>
#include <stop_token>

class TaskExecutionContext;

class SerialTaskExecution final
{
  public:
	static TaskExecutionCompletion Execute(
	    const TaskGraphStorage& graph,
	    TaskExecutionContext& context,
	    std::uint64_t generation,
	    std::stop_token cancellation);

  private:
	class RunState;
};
