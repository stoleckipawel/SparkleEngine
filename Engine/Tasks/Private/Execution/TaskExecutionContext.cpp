#include "TaskExecutionContext.h"

#include <utility>

TaskExecutionContext::TaskExecutionContext() noexcept = default;

void TaskExecutionContextBinding::Bind(
    TaskExecutionContext& context,
    std::uint64_t generation,
    TaskLane lane,
    std::stop_token cancellation) noexcept
{
	context.m_executionGeneration = generation;
	context.m_lane = lane;
	context.m_cancellation = std::move(cancellation);
}
