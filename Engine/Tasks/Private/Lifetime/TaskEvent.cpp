#include "TaskEvent.h"

#include "Core/Public/Diagnostics/Verify.h"

#include <atomic>
#include <condition_variable>
#include <limits>
#include <mutex>
#include <stop_token>
#include <utility>

static const auto g_taskEventLogger = Logging::GetOrCreateLogger("Tasks.Event");

struct TaskEvent::State final
{
	class WaitRegistration;
	static std::uint64_t IssueIdentity() noexcept;

	std::uint64_t Identity = IssueIdentity();
	std::uint64_t Generation = 1;
	std::mutex Mutex;
	std::condition_variable Condition;
	bool Signalled = false;
	std::uint32_t WaiterCount = 0;
	TaskResult Result = TaskResult::Success();
};

std::uint64_t TaskEvent::State::IssueIdentity() noexcept
{
	static std::atomic_uint64_t nextIdentity{1};
	std::uint64_t identity = nextIdentity.load(std::memory_order_relaxed);
	for (;;)
	{
		if (identity == 0)
		{
			Diagnostics::Fatal(g_taskEventLogger, __FILE__, __LINE__, "TaskEvent identity exhausted.");
		}

		const std::uint64_t next = identity == (std::numeric_limits<std::uint64_t>::max)() ? 0 : identity + 1;
		if (nextIdentity.compare_exchange_weak(identity, next, std::memory_order_relaxed, std::memory_order_relaxed))
		{
			return identity;
		}
	}
}

class TaskEvent::State::WaitRegistration final
{
public:
	explicit WaitRegistration(State& state) noexcept :
	    m_state(state)
	{
		++m_state.WaiterCount;
	}
	~WaitRegistration() { --m_state.WaiterCount; }

	WaitRegistration(const WaitRegistration&) = delete;
	WaitRegistration& operator=(const WaitRegistration&) = delete;

private:
	State& m_state;
};

TaskEventToken::TaskEventToken() noexcept = default;

TaskEventToken::TaskEventToken(std::uint64_t identity, std::uint64_t generation) noexcept :
    m_identity(identity),
    m_generation(generation)
{
}

bool TaskEventToken::operator==(const TaskEventToken&) const noexcept = default;

TaskEvent::TaskEvent() :
    m_state(std::make_shared<State>())
{
}

TaskEvent::~TaskEvent() = default;

TaskEventToken TaskEvent::GetToken() const noexcept
{
	std::lock_guard lock(m_state->Mutex);
	return TaskEventToken(m_state->Identity, m_state->Generation);
}

bool TaskEvent::Signal(TaskEventToken token, TaskResult result) noexcept
{
	{
		std::lock_guard lock(m_state->Mutex);
		if (!token || token.m_identity != m_state->Identity || token.m_generation != m_state->Generation || m_state->Signalled)
		{
			return false;
		}
		m_state->Result = std::move(result);
		m_state->Signalled = true;
	}
	m_state->Condition.notify_all();
	return true;
}

TaskEventToken TaskEvent::Reset() noexcept
{
	std::lock_guard lock(m_state->Mutex);
	if (!m_state->Signalled || m_state->WaiterCount != 0 || m_state->Generation == std::numeric_limits<std::uint64_t>::max())
	{
		return {};
	}
	++m_state->Generation;
	m_state->Signalled = false;
	m_state->Result = TaskResult::Success();
	return TaskEventToken(m_state->Identity, m_state->Generation);
}

TaskResult TaskEvent::Wait(TaskEventToken token, TaskExecutionContext& context)
{
	if (context.GetLane() != TaskLane::BlockingIo)
	{
		return TaskResult::Failure("TaskEvent wait is restricted to the BlockingIo lane.");
	}

	const std::shared_ptr state = m_state;
	std::stop_callback cancellationWake(
	    context.GetCancellationToken(),
	    [state]
	    {
		    std::lock_guard lock(state->Mutex);
		    state->Condition.notify_all();
	    });
	std::unique_lock lock(state->Mutex);
	State::WaitRegistration waiterRegistration(*state);
	state->Condition.wait(
	    lock,
	    [&]
	    {
		    return context.IsCancellationRequested() || token.m_identity != state->Identity || token.m_generation != state->Generation
		        || state->Signalled;
	    });

	if (context.IsCancellationRequested())
	{
		return TaskResult::Cancelled("TaskEvent wait was cancelled.");
	}
	if (!token || token.m_identity != state->Identity || token.m_generation != state->Generation)
	{
		return TaskResult::Failure("TaskEvent token is stale or belongs to another event.");
	}
	return state->Result;
}
