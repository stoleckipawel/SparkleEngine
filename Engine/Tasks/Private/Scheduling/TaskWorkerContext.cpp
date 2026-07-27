#include "TaskWorkerContext.h"

thread_local const void* TaskWorkerContext::s_executorIdentity = nullptr;

void TaskWorkerContext::Enter(const void* executorIdentity) noexcept
{
	s_executorIdentity = executorIdentity;
}

void TaskWorkerContext::Leave() noexcept
{
	s_executorIdentity = nullptr;
}

bool TaskWorkerContext::IsWorkerFor(const void* executorIdentity) noexcept
{
	return executorIdentity != nullptr && s_executorIdentity == executorIdentity;
}
