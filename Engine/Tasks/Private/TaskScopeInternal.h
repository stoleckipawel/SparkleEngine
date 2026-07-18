#pragma once

#include "TaskExecutorInternal.h"
#include "TaskScope.h"

#include <condition_variable>
#include <mutex>
#include <thread>
#include <vector>

struct TaskScope::State final : std::enable_shared_from_this<TaskScope::State>
{
	explicit State(TaskScopeDesc desc);

	bool RegisterExecution(const std::shared_ptr<TaskExecution::State>& execution);
	bool RegisterChild(const std::shared_ptr<State>& child);
	void ExecutionSettled();
	void ChildSettled();
	void Cancel() noexcept;
	bool JoinFor(std::chrono::milliseconds timeout);
	void CancelAndJoin() noexcept;
	bool IsCancellationRequested() const noexcept;
	bool IsSettled() const noexcept;
	void NotifyParentIfSettled(std::unique_lock<std::mutex>& lock);

	TaskScopeDesc Desc;
	std::thread::id OwnerThread = std::this_thread::get_id();
	mutable std::mutex Mutex;
	std::condition_variable Condition;
	std::weak_ptr<State> Parent;
	std::vector<std::weak_ptr<State>> Children;
	std::vector<std::weak_ptr<TaskExecution::State>> Executions;
	std::uint32_t ActiveExecutions = 0;
	std::uint32_t OpenChildren = 0;
	bool Closed = false;
	bool CancellationRequested = false;
	bool Settled = false;
	bool ParentNotified = false;
};
