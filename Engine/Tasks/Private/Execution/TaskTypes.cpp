#include "TaskTypes.h"

TaskName::TaskName() = default;

TaskName::TaskName(std::string_view value) : m_value(value) {}

bool TaskName::operator==(const TaskName&) const noexcept = default;

TaskResult::TaskResult() noexcept = default;

TaskResult::TaskResult(TaskOutcome outcome, std::string_view message) :
	m_outcome(outcome), m_message(message.substr(0, MaximumMessageLength))
{
}

TaskResult TaskResult::Success() noexcept
{
	return TaskResult(TaskOutcome::Succeeded, {});
}

TaskResult TaskResult::Failure(std::string_view message)
{
	return TaskResult(TaskOutcome::Failed, message);
}

TaskResult TaskResult::Cancelled(std::string_view reason)
{
	return TaskResult(TaskOutcome::Cancelled, reason);
}

bool TaskResult::operator==(const TaskResult&) const noexcept = default;
