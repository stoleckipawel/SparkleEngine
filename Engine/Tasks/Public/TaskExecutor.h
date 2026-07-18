#pragma once

#include "TaskExecution.h"
#include "TaskExecutionContext.h"

#include <cstdint>

struct TaskExecutorConfig final
{
	std::uint32_t MaximumTasksPerExecution = 1'024;
	std::uint32_t MaximumEdgesPerExecution = 4'096;
};

class SPARKLE_TASKS_API TaskExecutor final
{
  public:
	explicit TaskExecutor(TaskExecutorConfig config = {}) noexcept;

	TaskExecutor(const TaskExecutor&) = delete;
	TaskExecutor& operator=(const TaskExecutor&) = delete;
	TaskExecutor(TaskExecutor&&) = delete;
	TaskExecutor& operator=(TaskExecutor&&) = delete;

	TaskExecution Submit(const CompiledTaskGraph& graph, TaskExecutionContext& context);
	TaskExecution Submit(TaskDesc desc, TaskFunction function, TaskExecutionContext& context);

  private:
	TaskExecutorConfig m_config;
	std::uint64_t m_nextExecutionGeneration = 1;
};
