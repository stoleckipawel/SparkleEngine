#pragma once

#include "TasksAPI.h"

#include <chrono>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>

class TaskExecutor;

enum class TaskScopeKind : std::uint8_t
{
	Application,
	World,
	Document,
	AssetGeneration,
	Frame,
	ToolInvocation
};

struct TaskScopeDesc final
{
	TaskScopeKind Kind = TaskScopeKind::Application;
	std::string Name;
};

class SPARKLE_TASKS_API TaskScope final
{
public:
	explicit TaskScope(TaskScopeDesc desc, TaskScope* parent = nullptr);
	~TaskScope();

	TaskScope(const TaskScope&) = delete;
	TaskScope& operator=(const TaskScope&) = delete;
	TaskScope(TaskScope&&) = delete;
	TaskScope& operator=(TaskScope&&) = delete;

	void Cancel() noexcept;
	bool JoinFor(std::chrono::milliseconds timeout);
	bool IsCancellationRequested() const noexcept;
	bool IsSettled() const noexcept;
	TaskScopeKind GetKind() const noexcept;
	std::string_view GetName() const noexcept;

private:
	friend class TaskExecutor;
	struct State;
	std::shared_ptr<State> m_state;
};
