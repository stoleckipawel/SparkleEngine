#include "TaskExecutor.h"

#include "TaskExecutorInternal.h"
#include "TaskProfiler.h"
#include "TaskScopeInternal.h"

#include "Core/Public/Threading/ThreadOwnership.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace
{
	constexpr std::size_t TaskLaneCount = 3;
	thread_local const void* g_currentTaskExecutor = nullptr;

	std::size_t LaneIndex(TaskLane lane) noexcept
	{
		return static_cast<std::size_t>(lane);
	}

	std::string_view LaneName(TaskLane lane) noexcept
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

	TaskDetail::CompletedTaskExecution RejectedExecution(std::uint64_t generation, std::string_view reason)
	{
		TaskDetail::CompletedTaskExecution execution;
		execution.Generation = generation;
		execution.Status = TaskExecutionStatus::Rejected;
		execution.Result = TaskResult::Failure(reason);
		return execution;
	}
}

bool TaskDetail::IsExecutorWorker(const void* executorIdentity) noexcept
{
	return executorIdentity != nullptr && g_currentTaskExecutor == executorIdentity;
}

struct TaskExecutor::Implementation final
{
	enum class LifecycleState : std::uint8_t
	{
		Accepting,
		Draining,
		Cancelling,
		Stopping,
		Stopped
	};

	struct RunState;

	struct ReadyTask final
	{
		std::shared_ptr<RunState> Run;
		std::uint32_t TaskIndex = 0;
	};

	struct alignas(64) Worker final
	{
		TaskLane Lane = TaskLane::FrameCritical;
		std::uint32_t LaneWorkerIndex = 0;
		std::mutex QueueMutex;
		std::deque<ReadyTask> ReadyQueue;
		std::thread Thread;
	};

	struct LaneState final
	{
		std::mutex InjectionMutex;
		std::deque<ReadyTask> InjectionQueue;
		std::mutex WorkMutex;
		std::condition_variable WorkCondition;
		std::uint64_t WorkEpoch = 0;
		bool StopWorkers = false;
		std::vector<Worker*> Workers;
	};

	struct RuntimeTaskState final
	{
		std::atomic_uint32_t RemainingPrerequisites{0};
		std::atomic_uint32_t UnfinishedCount{1};
		std::atomic_bool ParentBodyComplete{true};
		std::atomic_bool BlockedByPrerequisite{false};
		std::atomic_bool BlockedByParent{false};
		std::atomic_bool Scheduled{false};
		std::atomic_bool Terminal{false};
	};

	struct RunState final : std::enable_shared_from_this<RunState>
	{
		RunState(
		    Implementation& owner,
		    std::shared_ptr<const TaskDetail::CompiledTaskGraphData> graph,
		    TaskExecutionContext context,
		    std::shared_ptr<TaskExecution::State> execution) :
		    Owner(owner), Graph(std::move(graph)), Context(std::move(context)), Execution(std::move(execution)),
		    Generation(Execution->Data.Generation),
		    Runtime(std::make_unique<RuntimeTaskState[]>(Graph->Nodes.size())), TaskResults(Graph->Nodes.size()),
		    Settled(Graph->Nodes.size(), false)
		{
			for (std::uint32_t index = 0; index < Graph->Nodes.size(); ++index)
			{
				Runtime[index].RemainingPrerequisites.store(
				    static_cast<std::uint32_t>(Graph->Nodes[index].Prerequisites.size()),
				    std::memory_order_relaxed);
				Runtime[index].UnfinishedCount.store(
				    1u + static_cast<std::uint32_t>(Graph->Nodes[index].NestedChildren.size()),
				    std::memory_order_relaxed);
				Runtime[index].ParentBodyComplete.store(!Graph->Nodes[index].Parent.has_value(), std::memory_order_relaxed);
			}
		}

		void Start()
		{
			for (std::uint32_t dependent = 0; dependent < Graph->Nodes.size(); ++dependent)
			{
				for (const std::uint32_t prerequisite : Graph->Nodes[dependent].Prerequisites)
				{
					TaskDetail::RecordTaskDependency(Generation, prerequisite, dependent);
				}
			}
			if (Graph->Nodes.empty())
			{
				Finish();
				return;
			}
			for (std::uint32_t index = 0; index < Graph->Nodes.size(); ++index)
			{
				TrySchedule(index, nullptr);
			}
		}

		void Execute(std::uint32_t index, Worker& worker)
		{
			RuntimeTaskState& task = Runtime[index];
			const TaskDetail::CompiledTaskNode& node = Graph->Nodes[index];
			const std::stop_token cancellation = Execution->Cancellation.get_token();
			const auto taskStart = TaskDetail::BeginTaskProfile(
			    node.Desc, Generation, index, worker.LaneWorkerIndex);
			const bool blocked = task.BlockedByPrerequisite.load(std::memory_order_acquire) ||
			                     task.BlockedByParent.load(std::memory_order_acquire) || cancellation.stop_requested();
			TaskExecutionContext taskContext = Context;
			TaskDetail::TaskExecutionContextAccess::Bind(taskContext, Generation, node.Desc.Lane, cancellation);
			TaskResult bodyResult = blocked && node.Desc.CompletionPolicy == TaskCompletionPolicy::Normal
			                            ? TaskResult::Cancelled("Task execution was cancelled or a prerequisite did not succeed.")
			                            : TaskDetail::InvokeTask(node, taskContext);

			{
				std::lock_guard lock(ResultMutex);
				TaskResults[index] = bodyResult;
				if (bodyResult.Failed() && FirstFailureTaskName.empty())
				{
					FirstFailureTaskName = std::string(node.Desc.Name.Get());
					FirstFailure = bodyResult;
				}
			}
			if (bodyResult.WasCancelled())
			{
				ObservedCancellation.store(true, std::memory_order_release);
			}
			TaskDetail::EndTaskProfile(node.Desc, Generation, index, worker.LaneWorkerIndex, bodyResult, taskStart);

			for (const std::uint32_t childIndex : node.NestedChildren)
			{
				RuntimeTaskState& child = Runtime[childIndex];
				if (!bodyResult.Succeeded())
				{
					child.BlockedByParent.store(true, std::memory_order_release);
				}
				child.ParentBodyComplete.store(true, std::memory_order_release);
				TrySchedule(childIndex, &worker);
			}
			ReleaseUnfinished(index, &worker);
		}

		void TrySchedule(std::uint32_t index, Worker* preferredWorker)
		{
			RuntimeTaskState& task = Runtime[index];
			if (task.RemainingPrerequisites.load(std::memory_order_acquire) != 0 ||
			    !task.ParentBodyComplete.load(std::memory_order_acquire))
			{
				return;
			}
			bool expected = false;
			if (task.Scheduled.compare_exchange_strong(expected, true, std::memory_order_acq_rel))
			{
				const TaskLane lane = Graph->Nodes[index].Desc.Lane;
				Owner.Enqueue(
				    ReadyTask{.Run = shared_from_this(), .TaskIndex = index},
				    preferredWorker != nullptr && preferredWorker->Lane == lane ? preferredWorker : nullptr,
				    lane);
			}
		}

		void ReleaseUnfinished(std::uint32_t index, Worker* worker)
		{
			if (Runtime[index].UnfinishedCount.fetch_sub(1, std::memory_order_acq_rel) == 1)
			{
				CompleteLogical(index, worker);
			}
		}

		void CompleteLogical(std::uint32_t index, Worker* worker)
		{
			bool expected = false;
			if (!Runtime[index].Terminal.compare_exchange_strong(expected, true, std::memory_order_acq_rel))
			{
				return;
			}

			TaskResult completedResult;
			{
				std::lock_guard lock(ResultMutex);
				completedResult = TaskResults[index];
				Settled[index] = true;
			}

			for (const std::uint32_t dependentIndex : Graph->Nodes[index].Dependents)
			{
				RuntimeTaskState& dependent = Runtime[dependentIndex];
				if (!completedResult.Succeeded())
				{
					dependent.BlockedByPrerequisite.store(true, std::memory_order_release);
				}
				if (dependent.RemainingPrerequisites.fetch_sub(1, std::memory_order_acq_rel) == 1)
				{
					TrySchedule(dependentIndex, worker);
				}
			}

			if (Graph->Nodes[index].Parent.has_value())
			{
				const std::uint32_t parentIndex = *Graph->Nodes[index].Parent;
				if (!completedResult.Succeeded())
				{
					std::lock_guard lock(ResultMutex);
					if (TaskResults[parentIndex].Succeeded())
					{
						TaskResults[parentIndex] = completedResult;
					}
				}
				ReleaseUnfinished(parentIndex, worker);
			}

			if (SettledTaskCount.fetch_add(1, std::memory_order_acq_rel) + 1u == Graph->Nodes.size())
			{
				Finish();
			}
		}

		void Finish()
		{
			bool expected = false;
			if (!Finished.compare_exchange_strong(expected, true, std::memory_order_acq_rel))
			{
				return;
			}

			TaskDetail::CompletedTaskExecution completed;
			completed.Generation = Generation;
			completed.BuilderIdentity = Graph->BuilderIdentity;
			completed.BuilderGeneration = Graph->BuilderGeneration;
			completed.SettledTaskCount = SettledTaskCount.load(std::memory_order_acquire);
			{
				std::lock_guard lock(ResultMutex);
				completed.TaskResults = TaskResults;
				completed.Settled = Settled;
				completed.FirstFailureTaskName = FirstFailureTaskName;
				if (!FirstFailureTaskName.empty())
				{
					completed.Status = TaskExecutionStatus::Failed;
					completed.Result = FirstFailure;
				}
			}
			if (completed.Status == TaskExecutionStatus::Invalid && ObservedCancellation.load(std::memory_order_acquire))
			{
				completed.Status = TaskExecutionStatus::Cancelled;
				completed.Result = TaskResult::Cancelled("Task execution contained cancellation.");
			}
			else if (completed.Status == TaskExecutionStatus::Invalid)
			{
				completed.Status = TaskExecutionStatus::Succeeded;
				completed.Result = TaskResult::Success();
			}
			Execution->Publish(std::move(completed));
			Owner.OnExecutionSettled();
		}

		Implementation& Owner;
		std::shared_ptr<const TaskDetail::CompiledTaskGraphData> Graph;
		TaskExecutionContext Context;
		std::shared_ptr<TaskExecution::State> Execution;
		std::uint64_t Generation = 0;
		std::unique_ptr<RuntimeTaskState[]> Runtime;
		std::mutex ResultMutex;
		std::vector<TaskResult> TaskResults;
		std::vector<bool> Settled;
		std::string FirstFailureTaskName;
		TaskResult FirstFailure = TaskResult::Success();
		std::atomic_uint32_t SettledTaskCount{0};
		std::atomic_bool ObservedCancellation{false};
		std::atomic_bool Finished{false};
	};

	explicit Implementation(TaskExecutorConfig config) : Config(config)
	{
		const std::uint32_t totalWorkers = config.FrameCriticalWorkerCount + config.BackgroundWorkerCount + config.BlockingIoWorkerCount;
		const bool invalidSerialMix = config.FrameCriticalWorkerCount == 0 &&
		                              (config.BackgroundWorkerCount != 0 || config.BlockingIoWorkerCount != 0);
		if (totalWorkers > MaximumWorkerCount || invalidSerialMix || Config.MaximumTasksPerExecution == 0 ||
		    Config.MaximumTasksPerExecution > TaskGraphLimits::HardMaximumTasks ||
		    Config.MaximumEdgesPerExecution > TaskGraphLimits::HardMaximumEdges || Config.MaximumActiveExecutions == 0)
		{
			throw std::invalid_argument("TaskExecutorConfig contains an unsupported lane worker count or capacity.");
		}

		Workers.reserve(totalWorkers);
		Executions.reserve(Config.MaximumActiveExecutions);
		AddWorkers(TaskLane::FrameCritical, config.FrameCriticalWorkerCount);
		AddWorkers(TaskLane::Background, config.BackgroundWorkerCount);
		AddWorkers(TaskLane::BlockingIo, config.BlockingIoWorkerCount);
		try
		{
			for (const auto& worker : Workers)
			{
				worker->Thread = std::thread([this, worker = worker.get()] { WorkerMain(*worker); });
			}
		}
		catch (...)
		{
			RequestWorkerStop();
			JoinWorkers();
			throw;
		}
	}

	~Implementation()
	{
		Shutdown(TaskExecutorShutdownMode::Drain);
	}

	void AddWorkers(TaskLane lane, std::uint32_t count)
	{
		LaneState& laneState = Lanes[LaneIndex(lane)];
		laneState.Workers.reserve(count);
		for (std::uint32_t index = 0; index < count; ++index)
		{
			auto worker = std::make_unique<Worker>();
			worker->Lane = lane;
			worker->LaneWorkerIndex = index;
			laneState.Workers.push_back(worker.get());
			Workers.push_back(std::move(worker));
		}
	}

	std::uint32_t WorkerCount(TaskLane lane) const noexcept
	{
		return static_cast<std::uint32_t>(Lanes[LaneIndex(lane)].Workers.size());
	}

	std::shared_ptr<TaskExecution::State> Launch(
	    const CompiledTaskGraph& graph,
	    TaskExecutionContext context,
	    const std::shared_ptr<TaskScope::State>& scope)
	{
		const std::uint64_t generation = NextExecutionGeneration.fetch_add(1, std::memory_order_relaxed);
		auto execution = std::make_shared<TaskExecution::State>(generation);
		execution->JoinThread = scope ? scope->OwnerThread : std::this_thread::get_id();
		execution->ExecutorIdentity = this;
		TaskDetail::TaskExecutionContextAccess::Bind(
		    context, generation, TaskLane::FrameCritical, execution->Cancellation.get_token());

		if (g_currentTaskExecutor == this)
		{
			execution->Publish(RejectedExecution(generation, "A task worker cannot submit work to its own executor; use graph dependencies or nested tasks."));
			return execution;
		}
		if (scope && context.HasUserData() && !context.HasOwnedUserData())
		{
			execution->Publish(RejectedExecution(generation, "Scoped asynchronous launch requires owned or empty TaskExecutionContext data."));
			return execution;
		}
		if (!graph.IsValid())
		{
			execution->Publish(RejectedExecution(generation, graph.GetError().Message));
			return execution;
		}
		if (graph.GetTaskCount() > Config.MaximumTasksPerExecution || graph.GetEdgeCount() > Config.MaximumEdgesPerExecution)
		{
			execution->Publish(RejectedExecution(generation, "Compiled task graph exceeds this executor's bounded execution capacity."));
			return execution;
		}
		if (!Workers.empty())
		{
			for (const auto& node : graph.m_data->Nodes)
			{
				if (WorkerCount(node.Desc.Lane) == 0)
				{
					execution->Publish(RejectedExecution(generation, "Compiled task graph uses a lane with no configured workers."));
					return execution;
				}
			}
		}

		auto run = Workers.empty() ? std::shared_ptr<RunState>{}
		                           : std::make_shared<RunState>(*this, graph.m_data, std::move(context), execution);
		if (scope && !scope->RegisterExecution(execution))
		{
			execution->Publish(RejectedExecution(generation, "TaskScope is closed, settled, or used from a non-owner thread."));
			return execution;
		}

		{
			std::lock_guard lock(StateMutex);
			std::erase_if(Executions, [](const std::weak_ptr<TaskExecution::State>& item) { return item.expired(); });
			if (State != LifecycleState::Accepting)
			{
				execution->Publish(RejectedExecution(generation, "Task executor is no longer accepting submissions."));
				return execution;
			}
			if (ActiveExecutions >= Config.MaximumActiveExecutions)
			{
				execution->Publish(RejectedExecution(generation, "Task executor reached its active-execution capacity."));
				return execution;
			}
			++ActiveExecutions;
			Executions.emplace_back(execution);
		}

		if (Workers.empty())
		{
			try
			{
				auto completed = TaskDetail::ExecuteSerial(
				    *graph.m_data, context, generation, execution->Cancellation.get_token());
				execution->Publish(std::move(completed));
				OnExecutionSettled();
			}
			catch (...)
			{
				OnExecutionSettled();
				throw;
			}
		}
		else
		{
			run->Start();
		}
		return execution;
	}

	void Enqueue(ReadyTask task, Worker* preferredWorker, TaskLane lane)
	{
		LaneState& laneState = Lanes[LaneIndex(lane)];
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

	bool TryPopLocal(Worker& worker, ReadyTask& task)
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

	bool TryPopInjection(TaskLane lane, ReadyTask& task)
	{
		LaneState& laneState = Lanes[LaneIndex(lane)];
		std::lock_guard lock(laneState.InjectionMutex);
		if (laneState.InjectionQueue.empty())
		{
			return false;
		}
		task = std::move(laneState.InjectionQueue.front());
		laneState.InjectionQueue.pop_front();
		return true;
	}

	bool TrySteal(Worker& worker, ReadyTask& task)
	{
		const auto& laneWorkers = Lanes[LaneIndex(worker.Lane)].Workers;
		for (std::uint32_t offset = 1; offset < laneWorkers.size(); ++offset)
		{
			Worker& victim = *laneWorkers[(worker.LaneWorkerIndex + offset) % laneWorkers.size()];
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

	bool TryTakeWork(Worker& worker, ReadyTask& task)
	{
		return TryPopLocal(worker, task) || TryPopInjection(worker.Lane, task) || TrySteal(worker, task);
	}

	void WorkerMain(Worker& worker)
	{
		g_currentTaskExecutor = this;
		Threading::SetCurrentThreadRole(
		    "Sparkle.Task." + std::string(LaneName(worker.Lane)) + "." + std::to_string(worker.LaneWorkerIndex));
		LaneState& laneState = Lanes[LaneIndex(worker.Lane)];
		for (;;)
		{
			ReadyTask task;
			if (TryTakeWork(worker, task))
			{
				task.Run->Execute(task.TaskIndex, worker);
				continue;
			}
			std::unique_lock lock(laneState.WorkMutex);
			const std::uint64_t observedEpoch = laneState.WorkEpoch;
			if (TryTakeWork(worker, task))
			{
				lock.unlock();
				task.Run->Execute(task.TaskIndex, worker);
				continue;
			}
			laneState.WorkCondition.wait(
			    lock,
			    [&laneState, observedEpoch] { return laneState.StopWorkers || laneState.WorkEpoch != observedEpoch; });
			if (laneState.StopWorkers)
			{
				break;
			}
		}
		g_currentTaskExecutor = nullptr;
	}

	void OnExecutionSettled()
	{
		std::lock_guard lock(StateMutex);
		--ActiveExecutions;
		StateCondition.notify_all();
	}

	bool Shutdown(TaskExecutorShutdownMode mode) noexcept
	{
		if (g_currentTaskExecutor == this)
		{
			return false;
		}
		std::unique_lock shutdownLock(ShutdownMutex);
		std::vector<std::shared_ptr<TaskExecution::State>> executionsToCancel;
		{
			std::unique_lock lock(StateMutex);
			if (State == LifecycleState::Stopped)
			{
				return true;
			}
			if (State == LifecycleState::Accepting)
			{
				State = mode == TaskExecutorShutdownMode::Cancel ? LifecycleState::Cancelling : LifecycleState::Draining;
			}
			if (State == LifecycleState::Cancelling)
			{
				for (auto& item : Executions)
				{
					if (auto execution = item.lock())
					{
						executionsToCancel.push_back(std::move(execution));
					}
				}
			}
		}
		for (const auto& execution : executionsToCancel)
		{
			execution->RequestCancellation();
		}
		{
			std::unique_lock lock(StateMutex);
			StateCondition.wait(lock, [this] { return ActiveExecutions == 0; });
			State = LifecycleState::Stopping;
		}
		RequestWorkerStop();
		JoinWorkers();
		{
			std::lock_guard lock(StateMutex);
			State = LifecycleState::Stopped;
			Executions.clear();
		}
		StateCondition.notify_all();
		return true;
	}

	void RequestWorkerStop() noexcept
	{
		for (LaneState& lane : Lanes)
		{
			{
				std::lock_guard lock(lane.WorkMutex);
				lane.StopWorkers = true;
				++lane.WorkEpoch;
			}
			lane.WorkCondition.notify_all();
		}
	}

	void JoinWorkers() noexcept
	{
		for (const auto& worker : Workers)
		{
			if (worker->Thread.joinable())
			{
				worker->Thread.join();
			}
		}
	}

	static constexpr std::uint32_t MaximumWorkerCount = 256;
	TaskExecutorConfig Config;
	std::array<LaneState, TaskLaneCount> Lanes;
	std::vector<std::unique_ptr<Worker>> Workers;
	std::mutex StateMutex;
	std::condition_variable StateCondition;
	LifecycleState State = LifecycleState::Accepting;
	std::uint32_t ActiveExecutions = 0;
	std::vector<std::weak_ptr<TaskExecution::State>> Executions;
	std::mutex ShutdownMutex;
	std::atomic_uint64_t NextExecutionGeneration{1};
};

TaskExecutor::TaskExecutor(TaskExecutorConfig config) : m_implementation(std::make_unique<Implementation>(config)) {}

TaskExecutor::~TaskExecutor() = default;

TaskExecution TaskExecutor::Submit(const CompiledTaskGraph& graph, TaskExecutionContext& context)
{
	auto state = m_implementation->Launch(graph, context, {});
	{
		std::unique_lock lock(state->Mutex);
		state->Condition.wait(lock, [&state] { return state->Settled; });
	}
	return TaskExecution(std::move(state));
}

TaskExecution TaskExecutor::Submit(TaskDesc desc, TaskFunction function, TaskExecutionContext& context)
{
	TaskGraphBuilder builder(TaskGraphLimits{
	    .MaximumTasks = m_implementation->Config.MaximumTasksPerExecution,
	    .MaximumEdges = m_implementation->Config.MaximumEdgesPerExecution});
	builder.Add(std::move(desc), std::move(function));
	return Submit(builder.Compile(), context);
}

TaskExecution TaskExecutor::Launch(TaskScope& scope, const CompiledTaskGraph& graph, TaskExecutionContext context)
{
	return TaskExecution(m_implementation->Launch(graph, std::move(context), scope.m_state));
}

TaskExecution TaskExecutor::Launch(TaskScope& scope, TaskDesc desc, TaskFunction function, TaskExecutionContext context)
{
	TaskGraphBuilder builder(TaskGraphLimits{
	    .MaximumTasks = m_implementation->Config.MaximumTasksPerExecution,
	    .MaximumEdges = m_implementation->Config.MaximumEdgesPerExecution});
	builder.Add(std::move(desc), std::move(function));
	return Launch(scope, builder.Compile(), std::move(context));
}

bool TaskExecutor::Shutdown(TaskExecutorShutdownMode mode) noexcept
{
	return m_implementation->Shutdown(mode);
}

std::uint32_t TaskExecutor::GetWorkerCount(TaskLane lane) const noexcept
{
	return LaneIndex(lane) < TaskLaneCount ? m_implementation->WorkerCount(lane) : 0;
}
