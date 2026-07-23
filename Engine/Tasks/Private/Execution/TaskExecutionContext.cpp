#include "TaskExecutionContext.h"

TaskExecutionContext::TaskExecutionContext() noexcept = default;

void TaskDetail::TaskExecutionContextAccess::Bind(
    TaskExecutionContext& context,
    std::uint64_t generation,
    TaskLane lane,
    std::stop_token cancellation) noexcept
{
	context.m_executionGeneration = generation;
	context.m_lane = lane;
	context.m_cancellation = cancellation;
}
