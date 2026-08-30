#pragma once

#include "TasksAPI.h"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <string_view>

class TaskExecutionContext;

class SPARKLE_TASKS_API TaskName final
{
public:
	static constexpr std::size_t MaximumLength = 96;

	TaskName();
	explicit TaskName(std::string_view value);

	std::string_view Get() const noexcept { return m_value; }
	bool IsValid() const noexcept { return !m_value.empty() && m_value.size() <= MaximumLength; }

	bool operator==(const TaskName&) const noexcept;

private:
	std::string m_value;
};

enum class TaskCompletionPolicy : std::uint8_t
{
	Normal,
	Cleanup
};

enum class TaskLane : std::uint8_t
{
	FrameCritical,
	Background,
	BlockingIo
};

enum class TaskOutcome : std::uint8_t
{
	Succeeded,
	Failed,
	Cancelled
};

class SPARKLE_TASKS_API TaskResult final
{
public:
	static constexpr std::size_t MaximumMessageLength = 512;
	TaskResult() noexcept;

	static TaskResult Success() noexcept;
	static TaskResult Failure(std::string_view message);
	static TaskResult Cancelled(std::string_view reason = {});

	TaskOutcome GetOutcome() const noexcept { return m_outcome; }
	std::string_view GetMessage() const noexcept { return m_message; }
	bool Succeeded() const noexcept { return m_outcome == TaskOutcome::Succeeded; }
	bool Failed() const noexcept { return m_outcome == TaskOutcome::Failed; }
	bool WasCancelled() const noexcept { return m_outcome == TaskOutcome::Cancelled; }

	bool operator==(const TaskResult&) const noexcept;

private:
	TaskResult(TaskOutcome outcome, std::string_view message);

	TaskOutcome m_outcome = TaskOutcome::Succeeded;
	std::string m_message;
};

using TaskFunction = std::function<TaskResult(TaskExecutionContext&)>;

struct TaskDesc final
{
	TaskName Name;
	TaskLane Lane = TaskLane::FrameCritical;
	TaskCompletionPolicy CompletionPolicy = TaskCompletionPolicy::Normal;
};
