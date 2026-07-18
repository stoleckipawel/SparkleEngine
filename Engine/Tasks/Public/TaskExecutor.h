#pragma once

#include "TaskExecution.h"
#include "TaskExecutionContext.h"

#include <cstdint>
#include <memory>

class TaskScope;

struct TaskExecutorConfig final
{
	// All lane counts at zero retain the deterministic caller-thread reference path.
	std::uint32_t FrameCriticalWorkerCount = 0;
	std::uint32_t BackgroundWorkerCount = 0;
	std::uint32_t BlockingIoWorkerCount = 0;
	std::uint32_t MaximumTasksPerExecution = 1'024;
	std::uint32_t MaximumEdgesPerExecution = 4'096;
	std::uint32_t MaximumActiveExecutions = 64;
};

enum class TaskExecutorShutdownMode : std::uint8_t
{
	Drain,
	Cancel
};

class SPARKLE_TASKS_API TaskExecutor final
{
  public:
	explicit TaskExecutor(TaskExecutorConfig config = {});
	~TaskExecutor();

	TaskExecutor(const TaskExecutor&) = delete;
	TaskExecutor& operator=(const TaskExecutor&) = delete;
	TaskExecutor(TaskExecutor&&) = delete;
	TaskExecutor& operator=(TaskExecutor&&) = delete;

	// Submission is a host boundary and returns settled. Owned workers must express child work as graph edges.
	TaskExecution Submit(const CompiledTaskGraph& graph, TaskExecutionContext& context);
	TaskExecution Submit(TaskDesc desc, TaskFunction function, TaskExecutionContext& context);
	TaskExecution Launch(TaskScope& scope, const CompiledTaskGraph& graph, TaskExecutionContext context = {});
	TaskExecution Launch(TaskScope& scope, TaskDesc desc, TaskFunction function, TaskExecutionContext context = {});

	// Cancel prevents queued normal bodies from starting; running bodies finish and cleanup nodes still settle.
	bool Shutdown(TaskExecutorShutdownMode mode = TaskExecutorShutdownMode::Drain) noexcept;
	std::uint32_t GetWorkerCount(TaskLane lane) const noexcept;

  private:
	struct Implementation;
	std::unique_ptr<Implementation> m_implementation;
};
