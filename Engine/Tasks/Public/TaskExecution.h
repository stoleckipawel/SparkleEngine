#pragma once

#include "TaskGraph.h"

#include <string>
#include <cstdint>
#include <chrono>
#include <memory>
#include <optional>

enum class TaskExecutionStatus : std::uint8_t
{
	Invalid,
	Pending,
	Succeeded,
	Failed,
	Cancelled,
	Rejected
};

class SPARKLE_TASKS_API TaskExecution final
{
public:
	TaskExecution() noexcept;
	~TaskExecution();

	TaskExecution(TaskExecution&&) noexcept;
	TaskExecution& operator=(TaskExecution&&) noexcept;
	TaskExecution(const TaskExecution&) = delete;
	TaskExecution& operator=(const TaskExecution&) = delete;

	bool IsValid() const noexcept;
	bool IsSettled() const noexcept;
	bool WaitFor(std::chrono::milliseconds timeout) const;
	std::uint64_t GetGeneration() const noexcept;
	TaskExecutionStatus GetStatus() const noexcept;
	TaskResult GetResult() const;
	std::string GetFirstFailureTaskName() const;
	std::optional<TaskResult> GetTaskResult(TaskNodeHandle handle) const;
	std::uint32_t GetSettledTaskCount() const noexcept;

private:
	friend class TaskExecutor;
	friend class TaskScope;
	struct State;

	explicit TaskExecution(std::shared_ptr<State> state) noexcept;

	std::shared_ptr<State> m_state;
};
