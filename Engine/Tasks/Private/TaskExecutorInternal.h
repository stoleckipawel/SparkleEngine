#pragma once

#include "TaskExecution.h"
#include "TaskGraphInternal.h"

#include <cstdint>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <stop_token>
#include <string>
#include <thread>
#include <vector>

namespace TaskDetail
{
	struct CompletedTaskExecution final
	{
		std::uint64_t Generation = 0;
		std::uint64_t BuilderIdentity = 0;
		std::uint32_t BuilderGeneration = 0;
		TaskExecutionStatus Status = TaskExecutionStatus::Invalid;
		TaskResult Result = TaskResult::Cancelled("Invalid task execution.");
		std::string FirstFailureTaskName;
		std::vector<TaskResult> TaskResults;
		std::vector<bool> Settled;
		std::uint32_t SettledTaskCount = 0;
	};

	TaskResult InvokeTask(const CompiledTaskNode& node, TaskExecutionContext& context);
	CompletedTaskExecution ExecuteSerial(
	    const CompiledTaskGraphData& graph,
	    TaskExecutionContext& context,
	    std::uint64_t generation,
	    std::stop_token cancellation);

	bool IsExecutorWorker(const void* executorIdentity) noexcept;
}

struct TaskExecution::State final
{
	explicit State(std::uint64_t generation = 0) { Data.Generation = generation; Data.Status = TaskExecutionStatus::Pending; }

	void Publish(TaskDetail::CompletedTaskExecution completed);
	void RequestCancellation() noexcept { Cancellation.request_stop(); }

	mutable std::mutex Mutex;
	std::condition_variable Condition;
	TaskDetail::CompletedTaskExecution Data;
	std::stop_source Cancellation;
	std::thread::id JoinThread;
	const void* ExecutorIdentity = nullptr;
	bool Settled = false;
	std::function<void()> OnSettled;
};
