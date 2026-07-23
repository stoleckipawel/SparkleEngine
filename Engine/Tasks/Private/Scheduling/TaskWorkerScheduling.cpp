#include "TaskExecutorRuntime.h"

#include "ScheduledTaskExecution.h"

#include "Core/Public/Threading/ThreadOwnership.h"

#include <string>

class TaskWorkerSchedulingOperations final
{
  public:
	inline static thread_local const void* g_currentTaskExecutor = nullptr;

	static std::size_t LaneIndex(TaskLane lane) noexcept
	{
		return static_cast<std::size_t>(lane);
	}

	static std::string_view LaneName(TaskLane lane) noexcept
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
};

bool TaskDetail::IsExecutorWorker(const void* executorIdentity) noexcept
{
	return executorIdentity != nullptr && TaskWorkerSchedulingOperations::g_currentTaskExecutor == executorIdentity;
}

void TaskExecutor::Implementation::Runtime::AddWorkers(TaskLane lane, std::uint32_t count)
{
	TaskLaneState& laneState = m_lanes[TaskWorkerSchedulingOperations::LaneIndex(lane)];
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
	const std::size_t index = TaskWorkerSchedulingOperations::LaneIndex(lane);
	return index < TaskLaneCount ? static_cast<std::uint32_t>(m_lanes[index].Workers.size()) : 0;
}

void TaskExecutor::Implementation::Runtime::Enqueue(ReadyTask task, TaskWorker* preferredWorker, TaskLane lane)
{
	TaskLaneState& laneState = m_lanes[TaskWorkerSchedulingOperations::LaneIndex(lane)];
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
	{
		std::lock_guard lock(laneState.WorkMutex);
		++laneState.WorkEpoch;
	}
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
	TaskLaneState& laneState = m_lanes[TaskWorkerSchedulingOperations::LaneIndex(lane)];
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
	const auto& laneWorkers = m_lanes[TaskWorkerSchedulingOperations::LaneIndex(worker.Lane)].Workers;
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

void TaskExecutor::Implementation::Runtime::WorkerMain(TaskWorker& worker)
{
	TaskWorkerSchedulingOperations::g_currentTaskExecutor = this;
	Threading::SetCurrentThreadRole(
	    "Sparkle.Task." + std::string(TaskWorkerSchedulingOperations::LaneName(worker.Lane)) + "." + std::to_string(worker.LaneWorkerIndex));
	TaskLaneState& laneState = m_lanes[TaskWorkerSchedulingOperations::LaneIndex(worker.Lane)];
	for (;;)
	{
		ReadyTask task;
		if (TryTakeWork(worker, task))
		{
			task.Execution->Execute(task.TaskIndex, worker);
			continue;
		}
		std::unique_lock lock(laneState.WorkMutex);
		const std::uint64_t observedEpoch = laneState.WorkEpoch;
		if (TryTakeWork(worker, task))
		{
			lock.unlock();
			task.Execution->Execute(task.TaskIndex, worker);
			continue;
		}
		laneState.WorkCondition.wait(
		    lock, [&laneState, observedEpoch] { return laneState.StopWorkers || laneState.WorkEpoch != observedEpoch; });
		if (laneState.StopWorkers)
		{
			break;
		}
	}
	TaskWorkerSchedulingOperations::g_currentTaskExecutor = nullptr;
}

void TaskExecutor::Implementation::Runtime::RequestWorkerStop() noexcept
{
	for (TaskLaneState& lane : m_lanes)
	{
		{
			std::lock_guard lock(lane.WorkMutex);
			lane.StopWorkers = true;
			++lane.WorkEpoch;
		}
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
