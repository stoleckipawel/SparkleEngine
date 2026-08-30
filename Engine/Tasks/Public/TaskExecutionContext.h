#pragma once

#include "TaskTypes.h"

#include <cstdint>
#include <memory>
#include <stop_token>
#include <typeinfo>

class TaskExecutor;
class TaskEvent;
struct TaskExecutionContextBinding;

class SPARKLE_TASKS_API TaskExecutionContext final
{
public:
	TaskExecutionContext() noexcept;

	template <typename T> explicit TaskExecutionContext(T& value) noexcept :
	    m_userData(&value),
	    m_userType(&typeid(T))
	{
	}
	template <typename T> explicit TaskExecutionContext(std::shared_ptr<T> value) noexcept :
	    m_userData(value.get()),
	    m_userType(&typeid(T)),
	    m_userOwner(std::move(value))
	{
	}

	template <typename T> T* TryGet() noexcept
	{
		return m_userType != nullptr && *m_userType == typeid(T) ? static_cast<T*>(m_userData) : nullptr;
	}

	template <typename T> const T* TryGet() const noexcept
	{
		return m_userType != nullptr && *m_userType == typeid(T) ? static_cast<const T*>(m_userData) : nullptr;
	}

	std::uint64_t GetExecutionGeneration() const noexcept { return m_executionGeneration; }
	TaskLane GetLane() const noexcept { return m_lane; }
	bool IsCancellationRequested() const noexcept { return m_cancellation.stop_requested(); }
	std::stop_token GetCancellationToken() const noexcept { return m_cancellation; }
	bool HasUserData() const noexcept { return m_userData != nullptr; }
	bool HasOwnedUserData() const noexcept { return m_userOwner != nullptr; }

private:
	friend class TaskExecutor;
	friend class TaskEvent;
	friend struct TaskExecutionContextBinding;

	void* m_userData = nullptr;
	const std::type_info* m_userType = nullptr;
	std::shared_ptr<void> m_userOwner;
	std::stop_token m_cancellation;
	std::uint64_t m_executionGeneration = 0;
	TaskLane m_lane = TaskLane::FrameCritical;
};

struct TaskExecutionContextBinding final
{
	static void Bind(TaskExecutionContext& context, std::uint64_t generation, TaskLane lane, std::stop_token cancellation) noexcept;
};
