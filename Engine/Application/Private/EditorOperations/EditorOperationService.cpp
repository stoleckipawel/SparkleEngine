#include "PCH.h"
#include "EditorOperations/EditorOperationService.h"

#include "Tasks/Public/TaskExecutor.h"
#include "Tasks/Public/TaskScope.h"

#include <exception>

struct EditorOperationService::ControlState final
{
	ControlState(TaskExecutor& executor, TaskScope& applicationScope) :
	    Executor(executor), Scope(TaskScopeDesc{TaskScopeKind::Document, "Editor operations"}, &applicationScope)
	{
	}
	TaskExecutor& Executor;
	TaskScope Scope;
	TaskExecution Execution;
	std::shared_ptr<ShaderRecookExecutionResult> ShaderRecookResult;
};

EditorOperationService::EditorOperationService(TaskExecutor& executor, TaskScope& applicationScope) :
    m_control(std::make_unique<ControlState>(executor, applicationScope))
{
}

EditorOperationService::~EditorOperationService()
{
	CancelDocument();
	(void) m_control->Scope.JoinFor(std::chrono::milliseconds::max());
}

void EditorOperationService::CancelDocument() noexcept
{
	m_control->Scope.Cancel();
}

bool EditorOperationService::StartShaderRecook(
    std::uint64_t requestId, std::uint64_t baselinePublicationId, ShaderRecookRequest request,
    std::string& outErrorMessage) noexcept
{
	if (m_control->Execution.IsValid() && !m_control->Execution.IsSettled())
	{
		outErrorMessage = "An editor operation is already active.";
		return false;
	}
	try
	{
		m_control->ShaderRecookResult = std::make_shared<ShaderRecookExecutionResult>();
		const auto result = m_control->ShaderRecookResult;
		TaskGraphBuilder graph;
		graph.Add(TaskDesc{TaskName("Run shader compiler"), TaskLane::BlockingIo},
		          [requestId, baselinePublicationId, request = std::move(request), result](TaskExecutionContext& context)
		          {
			          *result = ShaderRecookOperation::Execute(
			              requestId, baselinePublicationId, std::move(request), context.GetCancellationToken());
			          if (context.IsCancellationRequested()) return TaskResult::Cancelled("Editor shader recook was cancelled.");
			          return result->Process.Succeeded() ? TaskResult::Success()
			                                             : TaskResult::Failure("Shader compiler process failed.");
		          });
		m_control->Execution = m_control->Executor.Launch(m_control->Scope, graph.Compile());
		outErrorMessage.clear();
		return true;
	}
	catch (const std::exception& exception)
	{
		m_control->ShaderRecookResult.reset();
		outErrorMessage = exception.what();
	}
	catch (...)
	{
		m_control->ShaderRecookResult.reset();
		outErrorMessage = "Unknown editor operation launch failure.";
	}
	return false;
}

bool EditorOperationService::TryConsumeShaderRecook(ShaderRecookExecutionResult& outResult) noexcept
{
	if (!m_control->Execution.IsValid() || !m_control->Execution.IsSettled()) return false;
	outResult = m_control->ShaderRecookResult ? std::move(*m_control->ShaderRecookResult) : ShaderRecookExecutionResult{};
	m_control->ShaderRecookResult.reset();
	m_control->Execution = TaskExecution{};
	return true;
}
