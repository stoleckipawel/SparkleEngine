#include "TaskExecutionInternal.h"

#include "TaskExecutionContext.h"
#include "Profiling/TaskProfiler.h"

#include <exception>
#include <format>
#include <functional>
#include <queue>
#include <utility>

namespace TaskDetail
{
	namespace
	{
		struct SerialTaskState final
		{
			std::uint32_t RemainingPrerequisites = 0;
			std::uint32_t UnfinishedCount = 1;
			bool ParentBodyComplete = true;
			bool BlockedByPrerequisite = false;
			bool BlockedByParent = false;
			bool Scheduled = false;
			bool BodyComplete = false;
			bool Terminal = false;
			TaskResult AggregateResult = TaskResult::Success();
		};
	}

	TaskResult InvokeTask(const CompiledTaskNode& node, TaskExecutionContext& context)
	{
		if (!node.Function)
		{
			return TaskResult::Success();
		}

		try
		{
			return node.Function(context);
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

	CompletedTaskExecution ExecuteSerial(
	    const CompiledTaskGraphData& graph,
	    TaskExecutionContext& context,
	    std::uint64_t generation,
	    std::stop_token cancellation)
	{
		CompletedTaskExecution execution;
		execution.Generation = generation;
		execution.BuilderIdentity = graph.BuilderIdentity;
		execution.BuilderGeneration = graph.BuilderGeneration;
		execution.TaskResults.resize(graph.Nodes.size());
		execution.Settled.resize(graph.Nodes.size(), false);

		std::vector<SerialTaskState> runtime(graph.Nodes.size());
		for (std::uint32_t index = 0; index < graph.Nodes.size(); ++index)
		{
			runtime[index].RemainingPrerequisites = static_cast<std::uint32_t>(graph.Nodes[index].Prerequisites.size());
			runtime[index].UnfinishedCount += static_cast<std::uint32_t>(graph.Nodes[index].NestedChildren.size());
			runtime[index].ParentBodyComplete = !graph.Nodes[index].Parent.has_value();
			for (const std::uint32_t prerequisite : graph.Nodes[index].Prerequisites)
			{
				RecordTaskDependency(generation, prerequisite, index);
			}
		}

		std::priority_queue<std::uint32_t, std::vector<std::uint32_t>, std::greater<>> ready;
		auto trySchedule = [&](std::uint32_t index)
		{
			SerialTaskState& task = runtime[index];
			if (!task.Scheduled && !task.BodyComplete && task.RemainingPrerequisites == 0 && task.ParentBodyComplete)
			{
				task.Scheduled = true;
				ready.push(index);
			}
		};

		for (std::uint32_t index = 0; index < graph.Nodes.size(); ++index)
		{
			trySchedule(index);
		}

		bool observedCancellation = false;
		std::function<void(std::uint32_t)> completeTask;
		completeTask = [&](std::uint32_t index)
		{
			SerialTaskState& completed = runtime[index];
			if (completed.Terminal)
			{
				return;
			}
			completed.Terminal = true;
			execution.TaskResults[index] = completed.AggregateResult;
			execution.Settled[index] = true;
			++execution.SettledTaskCount;

			for (const std::uint32_t dependentIndex : graph.Nodes[index].Dependents)
			{
				SerialTaskState& dependent = runtime[dependentIndex];
				dependent.BlockedByPrerequisite |= !completed.AggregateResult.Succeeded();
				--dependent.RemainingPrerequisites;
				trySchedule(dependentIndex);
			}

			if (graph.Nodes[index].Parent.has_value())
			{
				const std::uint32_t parentIndex = *graph.Nodes[index].Parent;
				SerialTaskState& parent = runtime[parentIndex];
				if (!completed.AggregateResult.Succeeded() && parent.AggregateResult.Succeeded())
				{
					parent.AggregateResult = completed.AggregateResult;
				}
				if (--parent.UnfinishedCount == 0)
				{
					completeTask(parentIndex);
				}
			}
		};

		while (!ready.empty())
		{
			const std::uint32_t index = ready.top();
			ready.pop();
			SerialTaskState& task = runtime[index];
			const CompiledTaskNode& node = graph.Nodes[index];
			const auto taskStart = BeginTaskProfile(node.Desc, generation, index, 0);

			const bool blocked = task.BlockedByPrerequisite || task.BlockedByParent || cancellation.stop_requested();
			TaskExecutionContext taskContext = context;
			TaskExecutionContextAccess::Bind(taskContext, generation, node.Desc.Lane, cancellation);
			TaskResult bodyResult = blocked && node.Desc.CompletionPolicy == TaskCompletionPolicy::Normal
			                            ? TaskResult::Cancelled("A prerequisite or nested parent did not succeed.")
			                            : InvokeTask(node, taskContext);
			EndTaskProfile(node.Desc, generation, index, 0, bodyResult, taskStart);

			task.BodyComplete = true;
			task.AggregateResult = bodyResult;
			if (bodyResult.Failed() && execution.FirstFailureTaskName.empty())
			{
				execution.FirstFailureTaskName = std::string(node.Desc.Name.Get());
				execution.Result = bodyResult;
			}
			observedCancellation |= bodyResult.WasCancelled();

			for (const std::uint32_t childIndex : node.NestedChildren)
			{
				SerialTaskState& child = runtime[childIndex];
				child.ParentBodyComplete = true;
				child.BlockedByParent = !bodyResult.Succeeded();
				trySchedule(childIndex);
			}

			if (--task.UnfinishedCount == 0)
			{
				completeTask(index);
			}
		}

		if (execution.SettledTaskCount != graph.Nodes.size())
		{
			execution.Status = TaskExecutionStatus::Rejected;
			execution.Result = TaskResult::Failure("Compiled task graph could not settle all nodes in serial execution.");
		}
		else if (!execution.FirstFailureTaskName.empty())
		{
			execution.Status = TaskExecutionStatus::Failed;
		}
		else if (observedCancellation)
		{
			execution.Status = TaskExecutionStatus::Cancelled;
			execution.Result = TaskResult::Cancelled("Task execution contained cancellation.");
		}
		else
		{
			execution.Status = TaskExecutionStatus::Succeeded;
			execution.Result = TaskResult::Success();
		}

		return execution;
	}
}
