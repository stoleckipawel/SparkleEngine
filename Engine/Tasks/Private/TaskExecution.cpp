#include "TaskExecution.h"

#include "TaskExecutorInternal.h"

#include <utility>

TaskExecution::TaskExecution(std::unique_ptr<State> state) noexcept : m_state(std::move(state)) {}

TaskExecution::TaskExecution() noexcept = default;

TaskExecution::~TaskExecution() = default;

TaskExecution::TaskExecution(TaskExecution&&) noexcept = default;

TaskExecution& TaskExecution::operator=(TaskExecution&&) noexcept = default;

bool TaskExecution::IsValid() const noexcept
{
	return m_state != nullptr;
}

std::uint64_t TaskExecution::GetGeneration() const noexcept
{
	return m_state != nullptr ? m_state->Data.Generation : 0;
}

TaskExecutionStatus TaskExecution::GetStatus() const noexcept
{
	return m_state != nullptr ? m_state->Data.Status : TaskExecutionStatus::Invalid;
}

const TaskResult& TaskExecution::GetResult() const noexcept
{
	static const TaskResult invalidResult = TaskResult::Cancelled("Invalid task execution.");
	return m_state != nullptr ? m_state->Data.Result : invalidResult;
}

std::string_view TaskExecution::GetFirstFailureTaskName() const noexcept
{
	return m_state != nullptr ? std::string_view(m_state->Data.FirstFailureTaskName) : std::string_view{};
}

std::optional<TaskResult> TaskExecution::GetTaskResult(TaskNodeHandle handle) const
{
	if (m_state == nullptr)
	{
		return std::nullopt;
	}

	const auto& data = m_state->Data;
	std::uint32_t index = 0;
	if (!TaskDetail::TaskGraphAccess::Decode(
	        handle,
	        data.BuilderIdentity,
	        data.BuilderGeneration,
	        static_cast<std::uint32_t>(data.TaskResults.size()),
	        index) ||
	    !data.Settled[index])
	{
		return std::nullopt;
	}
	return data.TaskResults[index];
}

std::uint32_t TaskExecution::GetSettledTaskCount() const noexcept
{
	return m_state != nullptr ? m_state->Data.SettledTaskCount : 0;
}
