#pragma once

#include "Execution/TaskExecutionState.h"
#include "Lifetime/TaskScopeState.h"
#include "TaskExecutorImplementation.h"

#include <array>
#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <memory>
#include <mutex>
#include <span>
#include <string_view>
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
		std::mutex WakeMutex;
		std::condition_variable WakeCondition;
		std::uint64_t WakeEpoch = 0;
		bool WorkersStopping = false;
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

	static void ValidateConfiguration(const TaskExecutorConfig& config);
	static bool RejectExecution(const std::shared_ptr<TaskExecution::State>& execution, std::uint64_t generation, std::string_view reason);
	static std::size_t LaneIndex(TaskLane lane) noexcept;
	static const char* LaneName(TaskLane lane) noexcept;
	std::shared_ptr<TaskExecution::State> CreateExecution(std::uint64_t generation, const std::shared_ptr<TaskScope::State>& scope) const;
	bool ValidateLaunchRequest(
	    const CompiledTaskGraph& graph,
	    const TaskExecutionContext& context,
	    const std::shared_ptr<TaskScope::State>& scope,
	    const std::shared_ptr<TaskExecution::State>& execution,
	    std::uint64_t generation) const;
	bool ValidateWorkerLanes(
	    const CompiledTaskGraph& graph,
	    const std::shared_ptr<TaskExecution::State>& execution,
	    std::uint64_t generation) const;
	bool RegisterScopedExecution(
	    const std::shared_ptr<TaskScope::State>& scope,
	    const std::shared_ptr<TaskExecution::State>& execution,
	    std::uint64_t generation) const;
	bool AdmitExecution(const std::shared_ptr<TaskExecution::State>& execution, std::uint64_t generation);
	void StartExecution(
	    const CompiledTaskGraph& graph,
	    TaskExecutionContext context,
	    const std::shared_ptr<TaskExecution::State>& execution);
	void ExecuteSerial(
	    const CompiledTaskGraph& graph,
	    TaskExecutionContext& context,
	    const std::shared_ptr<TaskExecution::State>& execution);
	void AddWorkers(TaskLane lane, std::uint32_t count);
	void StartWorkers();
	bool TryPopLocal(TaskWorker& worker, ReadyTask& task);
	bool TryPopInjection(TaskLane lane, ReadyTask& task);
	bool TrySteal(TaskWorker& worker, ReadyTask& task);
	bool TryTakeWork(TaskWorker& worker, ReadyTask& task);
	bool WaitForWork(TaskWorker& worker, ReadyTask& task);
	void WorkerMain(TaskWorker& worker);
	std::vector<std::shared_ptr<TaskExecution::State>> BeginShutdown(TaskExecutorShutdownMode mode);
	void RequestCancellation(std::span<const std::shared_ptr<TaskExecution::State>> executions) noexcept;
	void WaitForActiveExecutions();
	void FinishShutdown();
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
