#pragma once

#include "TaskExecutionContext.h"

#include <cstdint>
#include <memory>

class SPARKLE_TASKS_API TaskEventToken final
{
  public:
	TaskEventToken() noexcept = default;
	bool IsValid() const noexcept { return m_identity != 0 && m_generation != 0; }
	explicit operator bool() const noexcept { return IsValid(); }
	bool operator==(const TaskEventToken&) const noexcept = default;

  private:
	friend class TaskEvent;
	TaskEventToken(std::uint64_t identity, std::uint64_t generation) noexcept : m_identity(identity), m_generation(generation) {}

	std::uint64_t m_identity = 0;
	std::uint64_t m_generation = 0;
};

class SPARKLE_TASKS_API TaskEvent final
{
  public:
	TaskEvent();
	~TaskEvent();

	TaskEvent(const TaskEvent&) = delete;
	TaskEvent& operator=(const TaskEvent&) = delete;
	TaskEvent(TaskEvent&&) = delete;
	TaskEvent& operator=(TaskEvent&&) = delete;

	TaskEventToken GetToken() const noexcept;
	bool Signal(TaskEventToken token, TaskResult result = TaskResult::Success()) noexcept;
	TaskEventToken Reset() noexcept;
	TaskResult Wait(TaskEventToken token, TaskExecutionContext& context);

  private:
	struct State;
	std::shared_ptr<State> m_state;
};
