#include "PCH.h"

#include "ShaderRecook/ShaderRecookExecutionService.h"

#include "Tasks/Public/TaskExecutor.h"
#include "Tasks/Public/TaskScope.h"

#include <exception>
#include <utility>

struct ShaderRecookExecutionService::ControlState final
{
	ControlState(TaskExecutor& executor, TaskScope& applicationScope) :
	    Executor(executor), Scope(TaskScopeDesc{TaskScopeKind::AssetGeneration, "Shader recook"}, &applicationScope)
	{
	}

	TaskExecutor& Executor;
	TaskScope Scope;
	TaskExecution Execution;
	std::shared_ptr<ShaderRecookExecutionResult> Result;
};

ShaderRecookExecutionService::ShaderRecookExecutionService(TaskExecutor& executor, TaskScope& applicationScope) :
    m_control(std::make_unique<ControlState>(executor, applicationScope))
{
}

ShaderRecookExecutionService::~ShaderRecookExecutionService()
{
	m_control->Scope.Cancel();
	m_control->Scope.JoinFor(std::chrono::milliseconds::max());
}

bool ShaderRecookExecutionService::Start(
    std::uint64_t requestId,
    std::uint64_t baselinePublicationId,
    ShaderRecookRequest request,
    std::string& outErrorMessage) noexcept
{
	if (m_control->Execution.IsValid() && !m_control->Execution.IsSettled())
	{
		outErrorMessage = "A shader recook execution is already active.";
		return false;
	}

	try
	{
		m_control->Result = std::make_shared<ShaderRecookExecutionResult>();
		const std::shared_ptr<ShaderRecookExecutionResult> result = m_control->Result;
		TaskGraphBuilder graph;
		graph.Add(
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
		m_control->Execution = m_control->Executor.Launch(m_control->Scope, graph.Compile());
		outErrorMessage.clear();
		return true;
	}
	catch (const std::exception& exception)
	{
		m_control->Result.reset();
		outErrorMessage = exception.what();
		return false;
	}
	catch (...)
	{
		m_control->Result.reset();
		outErrorMessage = "Unknown shader recook launch failure.";
		return false;
	}
}

bool ShaderRecookExecutionService::TryConsume(ShaderRecookExecutionResult& outResult) noexcept
{
	if (!m_control->Execution.IsValid() || !m_control->Execution.IsSettled())
		return false;
	outResult = m_control->Result ? std::move(*m_control->Result) : ShaderRecookExecutionResult{};
	m_control->Result.reset();
	m_control->Execution = TaskExecution{};
	return true;
}
