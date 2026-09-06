#include "PCH.h"
#include "EditorOperations/EditorOperationService.h"

#include "Tasks/Public/TaskExecutor.h"
#include "Tasks/Public/TaskScope.h"

#include <exception>

struct EditorOperationService::ControlState final
{
	ControlState(TaskExecutor& executor, TaskScope& applicationScope) :
	    Executor(executor),
	    Scope(TaskScopeDesc{TaskScopeKind::Document, "Editor operations"}, &applicationScope)
	{
	}
	TaskExecutor& Executor;
	TaskScope Scope;
	TaskExecution Execution;
	std::shared_ptr<ShaderRecookExecutionResult> ShaderRecookResult;
	TaskExecution CaptureExecution;
	std::shared_ptr<ViewportCaptureResult> CaptureResult;
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
    std::uint64_t requestId,
    std::uint64_t baselinePublicationId,
    ShaderRecookRequest request,
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
		graph.Add(
		    TaskDesc{TaskName("Run shader compiler"), TaskLane::BlockingIo},
		    [requestId, baselinePublicationId, request = std::move(request), result](TaskExecutionContext& context) mutable
		    {
			    *result =
			        ShaderRecookOperation::Execute(requestId, baselinePublicationId, std::move(request), context.GetCancellationToken());
			    if (result->Process.SettledSuccessfully())
				    return TaskResult::Success();
			    return context.IsCancellationRequested() ? TaskResult::Cancelled("Editor shader recook was cancelled.")
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
	if (!m_control->Execution.IsValid() || !m_control->Execution.IsSettled())
		return false;
	outResult = m_control->ShaderRecookResult ? std::move(*m_control->ShaderRecookResult) : ShaderRecookExecutionResult{};
	m_control->ShaderRecookResult.reset();
	m_control->Execution = TaskExecution{};
	return true;
}

bool EditorOperationService::StartViewportCaptureWrite(ViewportCaptureReadback readback, std::string& outErrorMessage) noexcept
{
	if (m_control->CaptureExecution.IsValid() && !m_control->CaptureExecution.IsSettled())
	{
		outErrorMessage = "A viewport capture is already being written.";
		return false;
	}
	try
	{
		m_control->CaptureResult = std::make_shared<ViewportCaptureResult>(readback.Result);
		const auto result = m_control->CaptureResult;
		TaskGraphBuilder graph;
		graph.Add(
		    TaskDesc{TaskName("Write viewport capture"), TaskLane::BlockingIo},
		    [readback = std::move(readback), result](TaskExecutionContext& context)
		    {
			    if (context.IsCancellationRequested())
			    {
				    return TaskResult::Cancelled("Viewport capture write was cancelled.");
			    }
			    const bool written = WriteViewportCaptureBmp(readback);
			    result->Status = written ? ViewportCaptureStatus::Succeeded : ViewportCaptureStatus::Failed;
			    if (!written)
			    {
				    result->FailureReason = "Viewport capture BMP encoding or write failed";
			    }
			    return written ? TaskResult::Success() : TaskResult::Failure(result->FailureReason);
		    });
		m_control->CaptureExecution = m_control->Executor.Launch(m_control->Scope, graph.Compile());
		outErrorMessage.clear();
		return true;
	}
	catch (const std::exception& exception)
	{
		m_control->CaptureResult.reset();
		outErrorMessage = exception.what();
	}
	catch (...)
	{
		m_control->CaptureResult.reset();
		outErrorMessage = "Unknown viewport capture launch failure.";
	}
	return false;
}

bool EditorOperationService::TryConsumeViewportCapture(ViewportCaptureResult& outResult) noexcept
{
	if (!m_control->CaptureExecution.IsValid() || !m_control->CaptureExecution.IsSettled())
	{
		return false;
	}
	outResult = m_control->CaptureResult ? std::move(*m_control->CaptureResult) : ViewportCaptureResult{};
	m_control->CaptureResult.reset();
	m_control->CaptureExecution = TaskExecution{};
	return true;
}
