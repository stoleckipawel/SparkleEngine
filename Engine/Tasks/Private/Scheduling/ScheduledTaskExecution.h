#pragma once

#include "Graph/TaskGraphInternal.h"
#include "TaskExecutorRuntime.h"

#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

class TaskExecutor::Implementation::Runtime::ScheduledTaskExecution final :
	public std::enable_shared_from_this<ScheduledTaskExecution>
{
  public:
	ScheduledTaskExecution(
	    Runtime& owner,
	    std::shared_ptr<const TaskDetail::CompiledTaskGraphData> graph,
	    TaskExecutionContext context,
	    std::shared_ptr<TaskExecution::State> execution);

	void Start();
	void Execute(std::uint32_t index, TaskWorker& worker);

  private:
	struct ScheduledTaskState final
	{
		std::atomic_uint32_t RemainingPrerequisites{0};
		std::atomic_uint32_t UnfinishedCount{1};
		std::atomic_bool ParentBodyComplete{true};
		std::atomic_bool BlockedByPrerequisite{false};
		std::atomic_bool BlockedByParent{false};
		std::atomic_bool Scheduled{false};
		std::atomic_bool Terminal{false};
	};

	void TrySchedule(std::uint32_t index, TaskWorker* preferredWorker);
	void ReleaseUnfinished(std::uint32_t index, TaskWorker* worker);
	void CompleteLogical(std::uint32_t index, TaskWorker* worker);
	void Finish();

	Runtime& m_owner;
	std::shared_ptr<const TaskDetail::CompiledTaskGraphData> m_graph;
	TaskExecutionContext m_context;
	std::shared_ptr<TaskExecution::State> m_execution;
	std::uint64_t m_generation = 0;
	std::unique_ptr<ScheduledTaskState[]> m_tasks;
	std::mutex m_resultMutex;
	std::vector<TaskResult> m_taskResults;
	std::vector<bool> m_settled;
	std::string m_firstFailureTaskName;
	TaskResult m_firstFailure = TaskResult::Success();
	std::atomic_uint32_t m_settledTaskCount{0};
	std::atomic_bool m_observedCancellation{false};
	std::atomic_bool m_finished{false};
};
