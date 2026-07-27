#include "TaskScope.h"

#include "TaskScopeState.h"

#include <cassert>
#include <stdexcept>
#include <utility>

TaskScope::State::State(TaskScopeDesc desc) : Desc(std::move(desc))
{
	if (Desc.Name.empty() || Desc.Name.size() > TaskName::MaximumLength)
	{
		throw std::invalid_argument("TaskScope name must be non-empty and at most 96 bytes.");
	}
}

bool TaskScope::State::RegisterExecution(const std::shared_ptr<TaskExecution::State>& execution)
{
	std::lock_guard lock(Mutex);
	if (Closed || Settled || std::this_thread::get_id() != OwnerThread)
	{
		return false;
	}
	++ActiveExecutions;
	Executions.emplace_back(execution);
	execution->JoinThread = OwnerThread;
	execution->OnSettled = [scope = weak_from_this()]
	{
		if (auto state = scope.lock())
		{
			state->ExecutionSettled();
		}
	};
	return true;
}

bool TaskScope::State::RegisterChild(const std::shared_ptr<State>& child)
{
	std::lock_guard lock(Mutex);
	if (Closed || Settled || std::this_thread::get_id() != OwnerThread || child->OwnerThread != OwnerThread)
	{
		return false;
	}
	++OpenChildren;
	Children.emplace_back(child);
	child->Parent = shared_from_this();
	return true;
}

void TaskScope::State::NotifyParentIfSettled(std::unique_lock<std::mutex>& lock)
{
	if (!Closed || ActiveExecutions != 0 || OpenChildren != 0 || Settled)
	{
		return;
	}
	Settled = true;
	Condition.notify_all();
	auto parent = ParentNotified ? std::shared_ptr<State>{} : Parent.lock();
	ParentNotified = true;
	lock.unlock();
	if (parent)
	{
		parent->ChildSettled();
	}
	lock.lock();
}

void TaskScope::State::ExecutionSettled()
{
	std::unique_lock lock(Mutex);
	if (ActiveExecutions > 0)
	{
		--ActiveExecutions;
	}
	NotifyParentIfSettled(lock);
}

void TaskScope::State::ChildSettled()
{
	std::unique_lock lock(Mutex);
	if (OpenChildren > 0)
	{
		--OpenChildren;
	}
	NotifyParentIfSettled(lock);
}

void TaskScope::State::Cancel() noexcept
{
	std::vector<std::shared_ptr<State>> children;
	std::vector<std::shared_ptr<TaskExecution::State>> executions;
	{
		std::unique_lock lock(Mutex);
		CancellationRequested = true;
		Closed = true;
		for (auto& child : Children)
		{
			if (auto state = child.lock())
			{
				children.push_back(std::move(state));
			}
		}
		for (auto& execution : Executions)
		{
			if (auto state = execution.lock())
			{
				executions.push_back(std::move(state));
			}
		}
		NotifyParentIfSettled(lock);
	}
	for (const auto& child : children)
	{
		child->Cancel();
	}
	for (const auto& execution : executions)
	{
		execution->RequestCancellation();
	}
}

bool TaskScope::State::JoinFor(std::chrono::milliseconds timeout)
{
	if (timeout < std::chrono::milliseconds::zero() || std::this_thread::get_id() != OwnerThread)
	{
		return false;
	}
	std::unique_lock lock(Mutex);
	Closed = true;
	NotifyParentIfSettled(lock);
	return Condition.wait_for(lock, timeout, [this] { return Settled; });
}

void TaskScope::State::CancelAndJoin() noexcept
{
	Cancel();
	if (std::this_thread::get_id() != OwnerThread)
	{
		return;
	}
	std::unique_lock lock(Mutex);
	Condition.wait(lock, [this] { return Settled; });
}

bool TaskScope::State::IsCancellationRequested() const noexcept
{
	std::lock_guard lock(Mutex);
	return CancellationRequested;
}

bool TaskScope::State::IsSettled() const noexcept
{
	std::lock_guard lock(Mutex);
	return Settled;
}

TaskScope::TaskScope(TaskScopeDesc desc, TaskScope* parent) : m_state(std::make_shared<State>(std::move(desc)))
{
	if (parent != nullptr && !parent->m_state->RegisterChild(m_state))
	{
		throw std::logic_error("TaskScope parent is closed, settled, or owned by another thread.");
	}
}

TaskScope::~TaskScope()
{
	const bool wasSettled = m_state->IsSettled();
	if (!wasSettled)
	{
		m_state->CancelAndJoin();
	}
	assert(wasSettled && "TaskScope owner must explicitly cancel/join or join before destruction.");
}

void TaskScope::Cancel() noexcept
{
	m_state->Cancel();
}

bool TaskScope::JoinFor(std::chrono::milliseconds timeout)
{
	return m_state->JoinFor(timeout);
}

bool TaskScope::IsCancellationRequested() const noexcept
{
	return m_state->IsCancellationRequested();
}

bool TaskScope::IsSettled() const noexcept
{
	return m_state->IsSettled();
}

TaskScopeKind TaskScope::GetKind() const noexcept
{
	return m_state->Desc.Kind;
}

std::string_view TaskScope::GetName() const noexcept
{
	return m_state->Desc.Name;
}
