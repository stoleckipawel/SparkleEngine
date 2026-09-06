#include "SerialTaskExecution.h"

#include "TaskExecutionContext.h"
#include "TaskFunctionInvoker.h"
#include "Profiling/TaskProfiler.h"

#include <functional>
#include <queue>
#include <utility>

class SerialTaskExecution::RunState final
{
public:
	RunState(const TaskGraphStorage& graph, TaskExecutionContext& context, std::uint64_t generation, std::stop_token cancellation);

	TaskExecutionCompletion Execute();

private:
	struct TaskState final
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

	void Initialize();
	void ScheduleInitialTasks();
	void TrySchedule(std::uint32_t index);
	void ExecuteReadyTasks();
	void ExecuteTask(std::uint32_t index);
	void CompleteTask(std::uint32_t index);
	void PublishFinalStatus();

	const TaskGraphStorage& m_graph;
	TaskExecutionContext& m_context;
	std::uint64_t m_generation = 0;
	std::stop_token m_cancellation;
	TaskExecutionCompletion m_completion;
	std::vector<TaskState> m_tasks;
	std::priority_queue<std::uint32_t, std::vector<std::uint32_t>, std::greater<>> m_ready;
	bool m_cancellationObserved = false;
};

SerialTaskExecution::RunState::RunState(
    const TaskGraphStorage& graph,
    TaskExecutionContext& context,
    std::uint64_t generation,
    std::stop_token cancellation) :
    m_graph(graph),
    m_context(context),
    m_generation(generation),
    m_cancellation(std::move(cancellation)),
    m_tasks(graph.Nodes.size())
{
	m_completion.Generation = generation;
	m_completion.BuilderIdentity = graph.BuilderIdentity;
	m_completion.BuilderGeneration = graph.BuilderGeneration;
	m_completion.TaskResults.resize(graph.Nodes.size());
	m_completion.Settled.resize(graph.Nodes.size(), false);
}

TaskExecutionCompletion SerialTaskExecution::RunState::Execute()
{
	Initialize();
	ScheduleInitialTasks();
	ExecuteReadyTasks();
	PublishFinalStatus();
	return std::move(m_completion);
}

void SerialTaskExecution::RunState::Initialize()
{
	for (std::uint32_t index = 0; index < m_graph.Nodes.size(); ++index)
	{
		const TaskGraphNode& node = m_graph.Nodes[index];
		TaskState& task = m_tasks[index];

		task.RemainingPrerequisites = static_cast<std::uint32_t>(node.Prerequisites.size());
		task.UnfinishedCount += static_cast<std::uint32_t>(node.NestedChildren.size());
		task.ParentBodyComplete = !node.Parent.has_value();

		for (const std::uint32_t prerequisite : node.Prerequisites)
		{
			TaskProfiler::RecordDependency(m_generation, prerequisite, index);
		}
	}
}

void SerialTaskExecution::RunState::ScheduleInitialTasks()
{
	for (std::uint32_t index = 0; index < m_graph.Nodes.size(); ++index)
	{
		TrySchedule(index);
	}
}

void SerialTaskExecution::RunState::TrySchedule(std::uint32_t index)
{
	TaskState& task = m_tasks[index];
	if (task.Scheduled || task.BodyComplete || task.RemainingPrerequisites != 0 || !task.ParentBodyComplete)
	{
		return;
	}

	task.Scheduled = true;
	m_ready.push(index);
}

void SerialTaskExecution::RunState::ExecuteReadyTasks()
{
	while (!m_ready.empty())
	{
		const std::uint32_t index = m_ready.top();
		m_ready.pop();
		ExecuteTask(index);
	}
}

void SerialTaskExecution::RunState::ExecuteTask(std::uint32_t index)
{
	TaskState& task = m_tasks[index];
	const TaskGraphNode& node = m_graph.Nodes[index];
	const TaskProfiler::TimePoint taskStart = TaskProfiler::Begin(node.Desc, m_generation, index, 0);

	const bool blocked = task.BlockedByPrerequisite || task.BlockedByParent || m_cancellation.stop_requested();
	TaskExecutionContext taskContext = m_context;
	TaskExecutionContextBinding::Bind(taskContext, m_generation, node.Desc.Lane, m_cancellation);
	TaskResult bodyResult = blocked && node.Desc.CompletionPolicy == TaskCompletionPolicy::Normal
	    ? TaskResult::Cancelled("A prerequisite or nested parent did not succeed.")
	    : TaskFunctionInvoker::Invoke(node, taskContext);

	TaskProfiler::End(node.Desc, m_generation, index, 0, bodyResult, taskStart);

	task.BodyComplete = true;
	task.AggregateResult = bodyResult;
	if (bodyResult.Failed() && m_completion.FirstFailureTaskName.empty())
	{
		m_completion.FirstFailureTaskName = std::string(node.Desc.Name.Get());
		m_completion.Result = bodyResult;
	}
	m_cancellationObserved |= bodyResult.WasCancelled();

	for (const std::uint32_t childIndex : node.NestedChildren)
	{
		TaskState& child = m_tasks[childIndex];
		child.ParentBodyComplete = true;
		child.BlockedByParent = !bodyResult.Succeeded();
		TrySchedule(childIndex);
	}

	if (--task.UnfinishedCount == 0)
	{
		CompleteTask(index);
	}
}

void SerialTaskExecution::RunState::CompleteTask(std::uint32_t index)
{
	TaskState& completed = m_tasks[index];
	if (completed.Terminal)
	{
		return;
	}

	completed.Terminal = true;
	m_completion.TaskResults[index] = completed.AggregateResult;
	m_completion.Settled[index] = true;
	++m_completion.SettledTaskCount;

	for (const std::uint32_t dependentIndex : m_graph.Nodes[index].Dependents)
	{
		TaskState& dependent = m_tasks[dependentIndex];
		dependent.BlockedByPrerequisite |= !completed.AggregateResult.Succeeded();
		--dependent.RemainingPrerequisites;
		TrySchedule(dependentIndex);
	}

	if (!m_graph.Nodes[index].Parent.has_value())
	{
		return;
	}

	const std::uint32_t parentIndex = *m_graph.Nodes[index].Parent;
	TaskState& parent = m_tasks[parentIndex];
	if (!completed.AggregateResult.Succeeded() && parent.AggregateResult.Succeeded())
	{
		parent.AggregateResult = completed.AggregateResult;
	}
	if (--parent.UnfinishedCount == 0)
	{
		CompleteTask(parentIndex);
	}
}

void SerialTaskExecution::RunState::PublishFinalStatus()
{
	if (m_completion.SettledTaskCount != m_graph.Nodes.size())
	{
		m_completion.Status = TaskExecutionStatus::Rejected;
		m_completion.Result = TaskResult::Failure("Compiled task graph could not settle all nodes in serial execution.");
	}
	else if (!m_completion.FirstFailureTaskName.empty())
	{
		m_completion.Status = TaskExecutionStatus::Failed;
	}
	else if (m_cancellationObserved)
	{
		m_completion.Status = TaskExecutionStatus::Cancelled;
		m_completion.Result = TaskResult::Cancelled("Task execution contained cancellation.");
	}
	else
	{
		m_completion.Status = TaskExecutionStatus::Succeeded;
		m_completion.Result = TaskResult::Success();
	}
}

TaskExecutionCompletion SerialTaskExecution::Execute(
    const TaskGraphStorage& graph,
    TaskExecutionContext& context,
    std::uint64_t generation,
    std::stop_token cancellation)
{
	return RunState(graph, context, generation, std::move(cancellation)).Execute();
}
