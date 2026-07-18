#pragma once

#include "Execution/TaskExecutionInternal.h"
#include "Lifetime/TaskScopeInternal.h"
#include "TaskExecutorImplementation.h"

#include <array>
#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

struct TaskExecutor::Implementation::Runtime final
{
	static constexpr std::size_t TaskLaneCount = 3;

	class ScheduledTaskExecution;

	struct ReadyTask final
	{
		std::shared_ptr<ScheduledTaskExecution> Execution;
		std::uint32_t TaskIndex = 0;
	};

	struct alignas(64) TaskWorker final
	{
		TaskLane Lane = TaskLane::FrameCritical;
		std::uint32_t LaneWorkerIndex = 0;
		std::mutex QueueMutex;
		std::deque<ReadyTask> ReadyQueue;
		std::thread Thread;
	};

	struct TaskLaneState final
	{
		std::mutex InjectionMutex;
		std::deque<ReadyTask> InjectionQueue;
		std::mutex WorkMutex;
		std::condition_variable WorkCondition;
		std::uint64_t WorkEpoch = 0;
		bool StopWorkers = false;
		std::vector<TaskWorker*> Workers;
	};

	explicit Runtime(TaskExecutorConfig config);
	~Runtime();

	Runtime(const Runtime&) = delete;
	Runtime& operator=(const Runtime&) = delete;

	std::shared_ptr<TaskExecution::State> Launch(
	    const CompiledTaskGraph& graph,
	    TaskExecutionContext context,
	    const std::shared_ptr<TaskScope::State>& scope);
	bool Shutdown(TaskExecutorShutdownMode mode) noexcept;
	std::uint32_t GetWorkerCount(TaskLane lane) const noexcept;
	const TaskExecutorConfig& GetConfig() const noexcept { return m_config; }

	void Enqueue(ReadyTask task, TaskWorker* preferredWorker, TaskLane lane);
	void OnExecutionSettled();

  private:
	enum class LifecycleState : std::uint8_t
	{
		Accepting,
		Draining,
		Cancelling,
		Stopping,
		Stopped
	};

	void AddWorkers(TaskLane lane, std::uint32_t count);
	bool TryPopLocal(TaskWorker& worker, ReadyTask& task);
	bool TryPopInjection(TaskLane lane, ReadyTask& task);
	bool TrySteal(TaskWorker& worker, ReadyTask& task);
	bool TryTakeWork(TaskWorker& worker, ReadyTask& task);
	void WorkerMain(TaskWorker& worker);
	void RequestWorkerStop() noexcept;
	void JoinWorkers() noexcept;

	static constexpr std::uint32_t MaximumWorkerCount = 256;
	TaskExecutorConfig m_config;
	std::array<TaskLaneState, TaskLaneCount> m_lanes;
	std::vector<std::unique_ptr<TaskWorker>> m_workers;
	std::mutex m_stateMutex;
	std::condition_variable m_stateCondition;
	LifecycleState m_lifecycle = LifecycleState::Accepting;
	std::uint32_t m_activeExecutions = 0;
	std::vector<std::weak_ptr<TaskExecution::State>> m_executions;
	std::mutex m_shutdownMutex;
	std::atomic_uint64_t m_nextExecutionGeneration{1};
};
