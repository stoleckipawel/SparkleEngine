#include "TaskExecutor.h"

#include "TaskExecutorInternal.h"

#include "Core/Public/Threading/ThreadOwnership.h"

#include <algorithm>
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
	thread_local const void* g_currentTaskExecutor = nullptr;

	TaskDetail::CompletedTaskExecution RejectedExecution(std::uint64_t generation, std::string_view reason)
	{
		TaskDetail::CompletedTaskExecution execution;
		execution.Generation = generation;
		execution.Status = TaskExecutionStatus::Rejected;
		execution.Result = TaskResult::Failure(reason);
		return execution;
	}
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
		std::mutex QueueMutex;
		std::deque<ReadyTask> ReadyQueue;
		std::thread Thread;
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
		    TaskExecutionContext& context,
		    std::uint64_t generation) :
		    Owner(owner), Graph(std::move(graph)), Context(context), Generation(generation), Runtime(std::make_unique<RuntimeTaskState[]>(Graph->Nodes.size())),
		    TaskResults(Graph->Nodes.size()), Settled(Graph->Nodes.size(), false)
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
			if (Graph->Nodes.empty())
			{
				Finish();
				return;
			}

			for (std::uint32_t index = 0; index < Graph->Nodes.size(); ++index)
			{
				TrySchedule(index, std::nullopt);
			}
		}

		void RequestCancellation() noexcept
		{
			CancelRequested.store(true, std::memory_order_release);
		}

		void Execute(std::uint32_t index, std::uint32_t workerIndex)
		{
			RuntimeTaskState& task = Runtime[index];
			const TaskDetail::CompiledTaskNode& node = Graph->Nodes[index];
			const bool blocked = task.BlockedByPrerequisite.load(std::memory_order_acquire) ||
			                     task.BlockedByParent.load(std::memory_order_acquire) ||
			                     CancelRequested.load(std::memory_order_acquire);
			TaskResult bodyResult = blocked && node.Desc.CompletionPolicy == TaskCompletionPolicy::Normal
			                            ? TaskResult::Cancelled("Task execution was cancelled or a prerequisite did not succeed.")
			                            : TaskDetail::InvokeTask(node, Context);

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

			for (const std::uint32_t childIndex : node.NestedChildren)
			{
				RuntimeTaskState& child = Runtime[childIndex];
				if (!bodyResult.Succeeded())
				{
					child.BlockedByParent.store(true, std::memory_order_release);
				}
				child.ParentBodyComplete.store(true, std::memory_order_release);
				TrySchedule(childIndex, workerIndex);
			}

			ReleaseUnfinished(index, workerIndex);
		}

		void TrySchedule(std::uint32_t index, std::optional<std::uint32_t> preferredWorker)
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
				Owner.Enqueue(ReadyTask{.Run = shared_from_this(), .TaskIndex = index}, preferredWorker);
			}
		}

		void ReleaseUnfinished(std::uint32_t index, std::uint32_t workerIndex)
		{
			const std::uint32_t previous = Runtime[index].UnfinishedCount.fetch_sub(1, std::memory_order_acq_rel);
			if (previous == 1)
			{
				CompleteLogical(index, workerIndex);
			}
		}

		void CompleteLogical(std::uint32_t index, std::uint32_t workerIndex)
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
				const std::uint32_t previous = dependent.RemainingPrerequisites.fetch_sub(1, std::memory_order_acq_rel);
				if (previous == 1)
				{
					TrySchedule(dependentIndex, workerIndex);
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
				ReleaseUnfinished(parentIndex, workerIndex);
			}

			const std::uint32_t settled = SettledTaskCount.fetch_add(1, std::memory_order_acq_rel) + 1u;
			if (settled == Graph->Nodes.size())
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
			Owner.OnExecutionSettled();
			{
				std::lock_guard lock(CompletionMutex);
				CompletionVisible = true;
			}
			CompletionCondition.notify_all();
		}

		TaskDetail::CompletedTaskExecution WaitAndCollect()
		{
			{
				std::unique_lock lock(CompletionMutex);
				CompletionCondition.wait(lock, [this] { return CompletionVisible; });
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
			return completed;
		}

		Implementation& Owner;
		std::shared_ptr<const TaskDetail::CompiledTaskGraphData> Graph;
		TaskExecutionContext& Context;
		std::uint64_t Generation = 0;
		std::unique_ptr<RuntimeTaskState[]> Runtime;
		std::mutex ResultMutex;
		std::vector<TaskResult> TaskResults;
		std::vector<bool> Settled;
		std::string FirstFailureTaskName;
		TaskResult FirstFailure = TaskResult::Success();
		std::atomic_uint32_t SettledTaskCount{0};
		std::atomic_bool ObservedCancellation{false};
		std::atomic_bool CancelRequested{false};
		std::atomic_bool Finished{false};
		std::mutex CompletionMutex;
		std::condition_variable CompletionCondition;
		bool CompletionVisible = false;
	};

	explicit Implementation(TaskExecutorConfig config) : Config(config)
	{
		if (Config.WorkerCount > MaximumWorkerCount || Config.MaximumTasksPerExecution == 0 ||
		    Config.MaximumTasksPerExecution > TaskGraphLimits::HardMaximumTasks ||
		    Config.MaximumEdgesPerExecution > TaskGraphLimits::HardMaximumEdges ||
		    Config.MaximumActiveExecutions == 0)
		{
			throw std::invalid_argument("TaskExecutorConfig contains an unsupported worker count or capacity.");
		}

		Workers.reserve(Config.WorkerCount);
		Runs.reserve(Config.MaximumActiveExecutions);
		try
		{
			for (std::uint32_t index = 0; index < Config.WorkerCount; ++index)
			{
				Workers.push_back(std::make_unique<Worker>());
			}
			for (std::uint32_t index = 0; index < Config.WorkerCount; ++index)
			{
				Workers[index]->Thread = std::thread([this, index] { WorkerMain(index); });
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

	TaskDetail::CompletedTaskExecution Execute(
	    const CompiledTaskGraph& graph,
	    TaskExecutionContext& context,
	    std::uint64_t generation)
	{
		if (g_currentTaskExecutor == this)
		{
			return RejectedExecution(generation, "A task worker cannot synchronously submit work to its own executor.");
		}
		if (!graph.IsValid())
		{
			return RejectedExecution(generation, graph.GetError().Message);
		}
		if (graph.GetTaskCount() > Config.MaximumTasksPerExecution || graph.GetEdgeCount() > Config.MaximumEdgesPerExecution)
		{
			return RejectedExecution(generation, "Compiled task graph exceeds this executor's bounded execution capacity.");
		}

		std::shared_ptr<RunState> run;
		if (!Workers.empty())
		{
			run = std::make_shared<RunState>(*this, graph.m_data, context, generation);
		}

		{
			std::lock_guard lock(StateMutex);
			std::erase_if(Runs, [](const std::weak_ptr<RunState>& run) { return run.expired(); });
			if (State != LifecycleState::Accepting)
			{
				return RejectedExecution(generation, "Task executor is no longer accepting submissions.");
			}
			if (ActiveExecutions >= Config.MaximumActiveExecutions)
			{
				return RejectedExecution(generation, "Task executor reached its active-execution capacity.");
			}
			++ActiveExecutions;
			if (run)
			{
				Runs.emplace_back(run);
			}
		}

		if (Workers.empty())
		{
			try
			{
				TaskDetail::CompletedTaskExecution completed = TaskDetail::ExecuteSerial(*graph.m_data, context, generation);
				OnExecutionSettled();
				return completed;
			}
			catch (...)
			{
				OnExecutionSettled();
				throw;
			}
		}

		run->Start();
		return run->WaitAndCollect();
	}

	void Enqueue(ReadyTask task, std::optional<std::uint32_t> preferredWorker)
	{
		if (preferredWorker.has_value())
		{
			Worker& worker = *Workers[*preferredWorker];
			std::lock_guard lock(worker.QueueMutex);
			worker.ReadyQueue.push_front(std::move(task));
		}
		else
		{
			std::lock_guard lock(InjectionMutex);
			InjectionQueue.push_back(std::move(task));
		}

		{
			std::lock_guard lock(WorkMutex);
			++WorkEpoch;
		}
		WorkCondition.notify_one();
	}

	bool TryPopLocal(std::uint32_t workerIndex, ReadyTask& task)
	{
		Worker& worker = *Workers[workerIndex];
		std::lock_guard lock(worker.QueueMutex);
		if (worker.ReadyQueue.empty())
		{
			return false;
		}
		task = std::move(worker.ReadyQueue.front());
		worker.ReadyQueue.pop_front();
		return true;
	}

	bool TryPopInjection(ReadyTask& task)
	{
		std::lock_guard lock(InjectionMutex);
		if (InjectionQueue.empty())
		{
			return false;
		}
		task = std::move(InjectionQueue.front());
		InjectionQueue.pop_front();
		return true;
	}

	bool TrySteal(std::uint32_t workerIndex, ReadyTask& task)
	{
		for (std::uint32_t offset = 1; offset < Workers.size(); ++offset)
		{
			Worker& victim = *Workers[(workerIndex + offset) % Workers.size()];
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

	bool TryTakeWork(std::uint32_t workerIndex, ReadyTask& task)
	{
		return TryPopLocal(workerIndex, task) || TryPopInjection(task) || TrySteal(workerIndex, task);
	}

	void WorkerMain(std::uint32_t workerIndex)
	{
		g_currentTaskExecutor = this;
		Threading::SetCurrentThreadRole("Sparkle.TaskWorker." + std::to_string(workerIndex));

		for (;;)
		{
			ReadyTask task;
			if (TryTakeWork(workerIndex, task))
			{
				task.Run->Execute(task.TaskIndex, workerIndex);
				continue;
			}

			std::unique_lock lock(WorkMutex);
			const std::uint64_t observedEpoch = WorkEpoch;
			if (TryTakeWork(workerIndex, task))
			{
				lock.unlock();
				task.Run->Execute(task.TaskIndex, workerIndex);
				continue;
			}
			WorkCondition.wait(lock, [this, observedEpoch] { return StopWorkers || WorkEpoch != observedEpoch; });
			if (StopWorkers)
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
		std::vector<std::shared_ptr<RunState>> runsToCancel;
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
				for (auto iterator = Runs.begin(); iterator != Runs.end();)
				{
					if (auto run = iterator->lock())
					{
						runsToCancel.push_back(std::move(run));
						++iterator;
					}
					else
					{
						iterator = Runs.erase(iterator);
					}
				}
			}
		}

		for (const auto& run : runsToCancel)
		{
			run->RequestCancellation();
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
			Runs.clear();
		}
		StateCondition.notify_all();
		return true;
	}

	void RequestWorkerStop() noexcept
	{
		{
			std::lock_guard lock(WorkMutex);
			StopWorkers = true;
			++WorkEpoch;
		}
		WorkCondition.notify_all();
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
	std::vector<std::unique_ptr<Worker>> Workers;
	std::mutex InjectionMutex;
	std::deque<ReadyTask> InjectionQueue;
	std::mutex WorkMutex;
	std::condition_variable WorkCondition;
	std::uint64_t WorkEpoch = 0;
	bool StopWorkers = false;
	std::mutex StateMutex;
	std::condition_variable StateCondition;
	LifecycleState State = LifecycleState::Accepting;
	std::uint32_t ActiveExecutions = 0;
	std::vector<std::weak_ptr<RunState>> Runs;
	std::mutex ShutdownMutex;
	std::atomic_uint64_t NextExecutionGeneration{1};
};

TaskExecutor::TaskExecutor(TaskExecutorConfig config) : m_implementation(std::make_unique<Implementation>(config)) {}

TaskExecutor::~TaskExecutor() = default;

TaskExecution TaskExecutor::Submit(const CompiledTaskGraph& graph, TaskExecutionContext& context)
{
	const std::uint64_t generation = m_implementation->NextExecutionGeneration.fetch_add(1, std::memory_order_relaxed);
	context.SetExecutionGeneration(generation);
	auto state = std::make_unique<TaskExecution::State>();
	state->Data = m_implementation->Execute(graph, context, generation);
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

bool TaskExecutor::Shutdown(TaskExecutorShutdownMode mode) noexcept
{
	return m_implementation->Shutdown(mode);
}

std::uint32_t TaskExecutor::GetWorkerCount() const noexcept
{
	return m_implementation->Config.WorkerCount;
}
