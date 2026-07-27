#pragma once

#include "TaskExecution.h"
#include "Graph/TaskGraphStorage.h"

#include <cstdint>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <stop_token>
#include <string>
#include <thread>
#include <vector>

struct TaskExecutionCompletion final
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

struct TaskExecution::State final
{
	explicit State(std::uint64_t generation = 0);

	void Publish(TaskExecutionCompletion completed);
	void RequestCancellation() noexcept;

	mutable std::mutex Mutex;
	std::condition_variable Condition;
	TaskExecutionCompletion Data;
	std::stop_source Cancellation;
	std::thread::id JoinThread;
	const void* ExecutorIdentity = nullptr;
	bool Settled = false;
	std::function<void()> OnSettled;
};
