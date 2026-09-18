#include "PCH.h"

#include "EditorOperations/EditorOperationRuntime.h"

#include <chrono>

EditorOperationRuntime::EditorOperationRuntime(TaskExecutor& executor, TaskScope& applicationScope) :
    m_executor(executor),
    m_scope(TaskScopeDesc{TaskScopeKind::Document, "Editor operations"}, &applicationScope)
{
}

EditorOperationRuntime::~EditorOperationRuntime()
{
	m_scope.Cancel();
	(void) m_scope.JoinFor(std::chrono::milliseconds::max());
}
