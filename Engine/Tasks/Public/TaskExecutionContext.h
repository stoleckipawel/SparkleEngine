#pragma once

#include "TasksAPI.h"

#include <cstdint>
#include <typeinfo>

class TaskExecutor;

class SPARKLE_TASKS_API TaskExecutionContext final
{
  public:
	TaskExecutionContext() noexcept = default;

	template <typename T> explicit TaskExecutionContext(T& value) noexcept : m_userData(&value), m_userType(&typeid(T)) {}

	template <typename T> T* TryGet() noexcept
	{
		return m_userType != nullptr && *m_userType == typeid(T) ? static_cast<T*>(m_userData) : nullptr;
	}

	template <typename T> const T* TryGet() const noexcept
	{
		return m_userType != nullptr && *m_userType == typeid(T) ? static_cast<const T*>(m_userData) : nullptr;
	}

	std::uint64_t GetExecutionGeneration() const noexcept { return m_executionGeneration; }

  private:
	friend class TaskExecutor;

	void SetExecutionGeneration(std::uint64_t generation) noexcept { m_executionGeneration = generation; }

	void* m_userData = nullptr;
	const std::type_info* m_userType = nullptr;
	std::uint64_t m_executionGeneration = 0;
};
