#pragma once

#include "TaskExecution.h"
#include "TaskGraphInternal.h"

#include <cstdint>
#include <string>
#include <vector>

namespace TaskDetail
{
	struct CompletedTaskExecution final
	{
		std::uint64_t Generation = 0;
		std::uint64_t BuilderIdentity = 0;
		std::uint32_t BuilderGeneration = 0;
		TaskExecutionStatus Status = TaskExecutionStatus::Invalid;
		TaskResult Result = TaskResult::Cancelled("Invalid task execution.");
		std::string FirstFailureTaskName;
		std::vector<TaskResult> TaskResults;
		std::vector<bool> Settled;
		std::uint32_t SettledTaskCount = 0;
	};

	TaskResult InvokeTask(const CompiledTaskNode& node, TaskExecutionContext& context);
	CompletedTaskExecution ExecuteSerial(
	    const CompiledTaskGraphData& graph,
	    TaskExecutionContext& context,
	    std::uint64_t generation);
}

struct TaskExecution::State final
{
	TaskDetail::CompletedTaskExecution Data;
};
