#include "PCH.h"

#include "ShaderRecook/ShaderRecookExecutionService.h"

#include "Tasks/Public/TaskExecutor.h"
#include "Tasks/Public/TaskScope.h"

#include <exception>
#include <utility>

struct ShaderRecookExecutionService::Implementation final
{
	TaskExecutor Executor{TaskExecutorConfig{
	    .FrameCriticalWorkerCount = 1,
	    .BackgroundWorkerCount = 1,
	    .BlockingIoWorkerCount = 1,
	    .MaximumActiveExecutions = 2}};
	TaskScope Scope{TaskScopeDesc{TaskScopeKind::Application, "Shader recook"}};
	TaskExecution Execution;
	std::shared_ptr<ShaderRecookExecutionResult> Result;
};

ShaderRecookExecutionService::ShaderRecookExecutionService() : m_implementation(std::make_unique<Implementation>()) {}

ShaderRecookExecutionService::~ShaderRecookExecutionService()
{
	m_implementation->Scope.Cancel();
	m_implementation->Executor.Shutdown(TaskExecutorShutdownMode::Cancel);
	m_implementation->Scope.JoinFor(std::chrono::milliseconds::zero());
}

bool ShaderRecookExecutionService::Start(
    std::uint64_t requestId,
    std::uint64_t baselinePublicationId,
    ShaderRecookRequest request,
    std::string& outErrorMessage) noexcept
{
	if (m_implementation->Execution.IsValid() && !m_implementation->Execution.IsSettled())
	{
		outErrorMessage = "A shader recook execution is already active.";
		return false;
	}

	try
	{
		m_implementation->Result = std::make_shared<ShaderRecookExecutionResult>();
		const std::shared_ptr<ShaderRecookExecutionResult> result = m_implementation->Result;
		TaskGraphBuilder graph;
		const TaskNodeHandle preparation = graph.Add(
		    TaskDesc{TaskName("Prepare shader recook"), TaskLane::Background},
		    [](TaskExecutionContext&)
		    {
			    return TaskResult::Success();
		    });
		const TaskNodeHandle process = graph.Add(
		    TaskDesc{TaskName("Run shader compiler"), TaskLane::BlockingIo},
		    [requestId, baselinePublicationId, request = std::move(request), result](TaskExecutionContext& context)
		    {
			    result->RequestId = requestId;
			    result->BaselinePublicationId = baselinePublicationId;
			    result->Request = request;
			    result->Process = ShaderCompilerProcess::RunCook(request, context.GetCancellationToken());
			    if (context.IsCancellationRequested())
				    return TaskResult::Cancelled("Shader recook was cancelled.");
			    return result->Process.Succeeded() ? TaskResult::Success() : TaskResult::Failure("Shader compiler process failed.");
		    });
		graph.DependsOn(process, preparation);
		m_implementation->Execution = m_implementation->Executor.Launch(m_implementation->Scope, graph.Compile());
		outErrorMessage.clear();
		return true;
	}
	catch (const std::exception& exception)
	{
		m_implementation->Result.reset();
		outErrorMessage = exception.what();
		return false;
	}
	catch (...)
	{
		m_implementation->Result.reset();
		outErrorMessage = "Unknown shader recook launch failure.";
		return false;
	}
}

bool ShaderRecookExecutionService::TryConsume(ShaderRecookExecutionResult& outResult) noexcept
{
	if (!m_implementation->Execution.IsValid() || !m_implementation->Execution.IsSettled())
		return false;
	outResult = m_implementation->Result ? std::move(*m_implementation->Result) : ShaderRecookExecutionResult{};
	m_implementation->Result.reset();
	m_implementation->Execution = TaskExecution{};
	return true;
}
