#include "TaskFunctionInvoker.h"

#include "TaskExecutionContext.h"

#include "Core/Public/Diagnostics/Error.h"

#include <exception>
#include <format>

TaskResult TaskFunctionInvoker::Invoke(const TaskGraphNode& node, TaskExecutionContext& context)
{
	if (!node.Function)
	{
		return TaskResult::Success();
	}

	try
	{
		return node.Function(context);
	}
	catch (const Diagnostics::Error& error)
	{
		return TaskResult::Failure(error.what());
	}
	catch (const std::exception& exception)
	{
		return TaskResult::Failure(std::format("Unhandled task exception: {}", exception.what()));
	}
	catch (...)
	{
		return TaskResult::Failure("Unhandled non-standard task exception.");
	}
}
