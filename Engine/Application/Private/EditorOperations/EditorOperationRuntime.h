#pragma once

#include "Tasks/Public/TaskScope.h"

class TaskExecutor;
template <typename TResult> class EditorOperationSlot;

// Shared lifetime only. Concrete editor features own their operation state and task bodies.
class EditorOperationRuntime final
{
public:
	EditorOperationRuntime(TaskExecutor& executor, TaskScope& applicationScope);
	~EditorOperationRuntime();

	EditorOperationRuntime(const EditorOperationRuntime&) = delete;
	EditorOperationRuntime& operator=(const EditorOperationRuntime&) = delete;

private:
	template <typename TResult> friend class EditorOperationSlot;

	TaskExecutor& GetExecutor() const noexcept { return m_executor; }
	TaskScope& GetScope() noexcept { return m_scope; }

	TaskExecutor& m_executor;
	TaskScope m_scope;
};
