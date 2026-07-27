#include "TaskExecutorRuntime.h"

#include "ScheduledTaskExecution.h"
#include "TaskWorkerContext.h"

#include "Core/Public/Threading/ThreadOwnership.h"

#include <string>

std::size_t TaskExecutor::Implementation::Runtime::LaneIndex(TaskLane lane) noexcept
{
	return static_cast<std::size_t>(lane);
}

const char* TaskExecutor::Implementation::Runtime::LaneName(TaskLane lane) noexcept
{
	switch (lane)
	{
		case TaskLane::FrameCritical:
			return "FrameCritical";
		case TaskLane::Background:
			return "Background";
		case TaskLane::BlockingIo:
			return "BlockingIo";
	}
	return "Invalid";
}

void TaskExecutor::Implementation::Runtime::AddWorkers(TaskLane lane, std::uint32_t count)
{
	TaskLaneState& laneState = m_lanes[LaneIndex(lane)];
	laneState.Workers.reserve(count);
	for (std::uint32_t index = 0; index < count; ++index)
	{
		auto worker = std::make_unique<TaskWorker>();
		worker->Lane = lane;
		worker->LaneWorkerIndex = index;
		laneState.Workers.push_back(worker.get());
		m_workers.push_back(std::move(worker));
	}
}

std::uint32_t TaskExecutor::Implementation::Runtime::GetWorkerCount(TaskLane lane) const noexcept
{
	const std::size_t index = LaneIndex(lane);
	return index < TaskLaneCount ? static_cast<std::uint32_t>(m_lanes[index].Workers.size()) : 0;
}

void TaskExecutor::Implementation::Runtime::Enqueue(ReadyTask task, TaskWorker* preferredWorker, TaskLane lane)
{
	TaskLaneState& laneState = m_lanes[LaneIndex(lane)];
	if (preferredWorker != nullptr)
	{
		std::lock_guard lock(preferredWorker->QueueMutex);
		preferredWorker->ReadyQueue.push_front(std::move(task));
	}
	else
	{
		std::lock_guard lock(laneState.InjectionMutex);
		laneState.InjectionQueue.push_back(std::move(task));
	}

	laneState.WorkEpoch.fetch_add(1, std::memory_order_release);
	laneState.WorkCondition.notify_one();
}

bool TaskExecutor::Implementation::Runtime::TryPopLocal(TaskWorker& worker, ReadyTask& task)
{
	std::lock_guard lock(worker.QueueMutex);
	if (worker.ReadyQueue.empty())
	{
		return false;
	}

	task = std::move(worker.ReadyQueue.front());
	worker.ReadyQueue.pop_front();
	return true;
}

bool TaskExecutor::Implementation::Runtime::TryPopInjection(TaskLane lane, ReadyTask& task)
{
	TaskLaneState& laneState = m_lanes[LaneIndex(lane)];
	std::lock_guard lock(laneState.InjectionMutex);
	if (laneState.InjectionQueue.empty())
	{
		return false;
	}

	task = std::move(laneState.InjectionQueue.front());
	laneState.InjectionQueue.pop_front();
	return true;
}

bool TaskExecutor::Implementation::Runtime::TrySteal(TaskWorker& worker, ReadyTask& task)
{
	const auto& laneWorkers = m_lanes[LaneIndex(worker.Lane)].Workers;
	for (std::uint32_t offset = 1; offset < laneWorkers.size(); ++offset)
	{
		TaskWorker& victim = *laneWorkers[(worker.LaneWorkerIndex + offset) % laneWorkers.size()];
		std::lock_guard lock(victim.QueueMutex);
		if (!victim.ReadyQueue.empty())
		{
			task = std::move(victim.ReadyQueue.back());
			victim.ReadyQueue.pop_back();
			return true;
		}
	}
	return false;
}

bool TaskExecutor::Implementation::Runtime::TryTakeWork(TaskWorker& worker, ReadyTask& task)
{
	return TryPopLocal(worker, task) || TryPopInjection(worker.Lane, task) || TrySteal(worker, task);
}

bool TaskExecutor::Implementation::Runtime::WaitForWork(TaskWorker& worker, ReadyTask& task)
{
	TaskLaneState& laneState = m_lanes[LaneIndex(worker.Lane)];
	for (;;)
	{
		const std::uint64_t observedEpoch = laneState.WorkEpoch.load(std::memory_order_acquire);
		if (TryTakeWork(worker, task))
		{
			return true;
		}

		std::unique_lock lock(laneState.WorkMutex);
		laneState.WorkCondition.wait(
		    lock,
		    [&laneState, observedEpoch]
		    {
			    return laneState.StopWorkers.load(std::memory_order_acquire) ||
			           laneState.WorkEpoch.load(std::memory_order_acquire) != observedEpoch;
		    });
		if (laneState.StopWorkers.load(std::memory_order_acquire))
		{
			return false;
		}
	}
}

void TaskExecutor::Implementation::Runtime::WorkerMain(TaskWorker& worker)
{
	TaskWorkerContext::Enter(this);
	Threading::SetCurrentThreadRole(
	    "Sparkle.Task." + std::string(LaneName(worker.Lane)) + "." +
	    std::to_string(worker.LaneWorkerIndex));

	ReadyTask task;
	while (TryTakeWork(worker, task) || WaitForWork(worker, task))
	{
		task.Execution->Execute(task.TaskIndex, worker);
	}

	TaskWorkerContext::Leave();
}

void TaskExecutor::Implementation::Runtime::RequestWorkerStop() noexcept
{
	for (TaskLaneState& lane : m_lanes)
	{
		lane.StopWorkers.store(true, std::memory_order_release);
		lane.WorkEpoch.fetch_add(1, std::memory_order_release);
		lane.WorkCondition.notify_all();
	}
}

void TaskExecutor::Implementation::Runtime::JoinWorkers() noexcept
{
	for (const auto& worker : m_workers)
	{
		if (worker->Thread.joinable())
		{
			worker->Thread.join();
		}
	}
}
