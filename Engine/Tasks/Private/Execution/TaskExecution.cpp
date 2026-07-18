#include "TaskExecution.h"

#include "TaskExecutionInternal.h"

#include <utility>

void TaskExecution::State::Publish(TaskDetail::CompletedTaskExecution completed)
{
	std::function<void()> onSettled;
	{
		std::lock_guard lock(Mutex);
		if (Settled)
		{
			return;
		}
		Data = std::move(completed);
		Settled = true;
		onSettled = std::move(OnSettled);
	}
	Condition.notify_all();
	if (onSettled)
	{
		onSettled();
	}
}

TaskExecution::TaskExecution(std::shared_ptr<State> state) noexcept : m_state(std::move(state)) {}

TaskExecution::TaskExecution() noexcept = default;

TaskExecution::~TaskExecution() = default;

TaskExecution::TaskExecution(TaskExecution&&) noexcept = default;

TaskExecution& TaskExecution::operator=(TaskExecution&&) noexcept = default;

bool TaskExecution::IsValid() const noexcept
{
	return m_state != nullptr;
}

bool TaskExecution::IsSettled() const noexcept
{
	if (m_state == nullptr)
	{
		return false;
	}
	std::lock_guard lock(m_state->Mutex);
	return m_state->Settled;
}

bool TaskExecution::WaitFor(std::chrono::milliseconds timeout) const
{
	if (m_state == nullptr || timeout < std::chrono::milliseconds::zero() ||
	    TaskDetail::IsExecutorWorker(m_state->ExecutorIdentity) || std::this_thread::get_id() != m_state->JoinThread)
	{
		return false;
	}
	std::unique_lock lock(m_state->Mutex);
	return m_state->Condition.wait_for(lock, timeout, [this] { return m_state->Settled; });
}

std::uint64_t TaskExecution::GetGeneration() const noexcept
{
	if (m_state == nullptr)
	{
		return 0;
	}
	std::lock_guard lock(m_state->Mutex);
	return m_state->Data.Generation;
}

TaskExecutionStatus TaskExecution::GetStatus() const noexcept
{
	if (m_state == nullptr)
	{
		return TaskExecutionStatus::Invalid;
	}
	std::lock_guard lock(m_state->Mutex);
	return m_state->Data.Status;
}

TaskResult TaskExecution::GetResult() const
{
	static const TaskResult invalidResult = TaskResult::Cancelled("Invalid task execution.");
	if (m_state == nullptr)
	{
		return invalidResult;
	}
	std::lock_guard lock(m_state->Mutex);
	return m_state->Data.Result;
}

std::string TaskExecution::GetFirstFailureTaskName() const
{
	if (m_state == nullptr)
	{
		return {};
	}
	std::lock_guard lock(m_state->Mutex);
	return m_state->Data.FirstFailureTaskName;
}

std::optional<TaskResult> TaskExecution::GetTaskResult(TaskNodeHandle handle) const
{
	if (m_state == nullptr)
	{
		return std::nullopt;
	}

	std::lock_guard lock(m_state->Mutex);
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
	if (m_state == nullptr)
	{
		return 0;
	}
	std::lock_guard lock(m_state->Mutex);
	return m_state->Data.SettledTaskCount;
}
