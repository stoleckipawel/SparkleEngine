#pragma once

#include "TaskGraph.h"

#include <cstdint>
#include <memory>
#include <optional>

enum class TaskExecutionStatus : std::uint8_t
{
	Invalid,
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
	std::uint64_t GetGeneration() const noexcept;
	TaskExecutionStatus GetStatus() const noexcept;
	const TaskResult& GetResult() const noexcept;
	std::string_view GetFirstFailureTaskName() const noexcept;
	std::optional<TaskResult> GetTaskResult(TaskNodeHandle handle) const;
	std::uint32_t GetSettledTaskCount() const noexcept;

  private:
	friend class TaskExecutor;
	struct State;

	explicit TaskExecution(std::unique_ptr<State> state) noexcept;

	std::unique_ptr<State> m_state;
};
