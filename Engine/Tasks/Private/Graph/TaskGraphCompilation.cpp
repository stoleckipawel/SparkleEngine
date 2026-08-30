#include "TaskGraphBuilderState.h"

#include <functional>
#include <queue>
#include <vector>

class TaskGraphBuilder::State::Compilation final
{
public:
	explicit Compilation(const State& builder) noexcept :
	    m_builder(builder)
	{
	}

	std::shared_ptr<TaskGraphStorage> Build() const
	{
		auto graph = std::make_shared<TaskGraphStorage>();
		CopyBuilderState(*graph);
		if (graph->Error)
		{
			return graph;
		}

		const std::size_t taskCount = graph->Nodes.size();
		std::vector<std::vector<std::uint32_t>> startAdjacency(taskCount);
		std::vector<std::vector<std::uint32_t>> completionAdjacency(taskCount);
		if (!BuildDependencyRelations(*graph, startAdjacency, completionAdjacency))
		{
			return graph;
		}

		RejectCycles(*graph, startAdjacency, completionAdjacency);
		return graph;
	}

private:
	static bool HasCycle(const std::vector<std::vector<std::uint32_t>>& adjacency)
	{
		std::vector<std::uint32_t> incoming(adjacency.size(), 0);
		for (const auto& successors : adjacency)
		{
			for (const std::uint32_t successor : successors)
			{
				++incoming[successor];
			}
		}

		std::priority_queue<std::uint32_t, std::vector<std::uint32_t>, std::greater<>> ready;
		for (std::uint32_t index = 0; index < incoming.size(); ++index)
		{
			if (incoming[index] == 0)
			{
				ready.push(index);
			}
		}

		std::uint32_t visited = 0;
		while (!ready.empty())
		{
			const std::uint32_t index = ready.top();
			ready.pop();
			++visited;

			for (const std::uint32_t successor : adjacency[index])
			{
				if (--incoming[successor] == 0)
				{
					ready.push(successor);
				}
			}
		}

		return visited != adjacency.size();
	}

	void CopyBuilderState(TaskGraphStorage& graph) const
	{
		graph.Error = m_builder.Error;
		graph.Limits = m_builder.Limits;
		graph.BuilderIdentity = m_builder.BuilderIdentity;
		graph.BuilderGeneration = m_builder.BuilderGeneration;
		graph.EdgeCount = m_builder.EdgeCount;
		graph.Nodes = m_builder.Nodes;
	}

	bool BuildDependencyRelations(
	    TaskGraphStorage& graph,
	    std::vector<std::vector<std::uint32_t>>& startAdjacency,
	    std::vector<std::vector<std::uint32_t>>& completionAdjacency) const
	{
		for (std::uint32_t taskIndex = 0; taskIndex < graph.Nodes.size(); ++taskIndex)
		{
			const TaskGraphNode& node = graph.Nodes[taskIndex];
			for (const std::uint32_t prerequisiteIndex : node.Prerequisites)
			{
				if (!ValidatePrerequisiteLane(graph, taskIndex, prerequisiteIndex))
				{
					return false;
				}
				startAdjacency[prerequisiteIndex].push_back(taskIndex);
				completionAdjacency[prerequisiteIndex].push_back(taskIndex);
			}

			if (!ValidateNestedLane(graph, taskIndex))
			{
				return false;
			}
			if (node.Parent.has_value())
			{
				startAdjacency[*node.Parent].push_back(taskIndex);
				completionAdjacency[taskIndex].push_back(*node.Parent);
			}
		}
		return true;
	}

	bool ValidatePrerequisiteLane(TaskGraphStorage& graph, std::uint32_t taskIndex, std::uint32_t prerequisiteIndex) const
	{
		const TaskGraphNode& node = graph.Nodes[taskIndex];
		const TaskGraphNode& prerequisite = graph.Nodes[prerequisiteIndex];
		if (node.Desc.Lane != TaskLane::FrameCritical || prerequisite.Desc.Lane == TaskLane::FrameCritical)
		{
			return true;
		}

		graph.Error = State::CreateError(
		    TaskGraphErrorCode::InvalidLaneDependency,
		    "A FrameCritical task cannot depend on Background or BlockingIo work.");
		return false;
	}

	bool ValidateNestedLane(TaskGraphStorage& graph, std::uint32_t taskIndex) const
	{
		const TaskGraphNode& node = graph.Nodes[taskIndex];
		if (!node.Parent.has_value())
		{
			return true;
		}

		const TaskGraphNode& parent = graph.Nodes[*node.Parent];
		if (node.Desc.Lane == parent.Desc.Lane
		    || (node.Desc.Lane != TaskLane::FrameCritical && parent.Desc.Lane != TaskLane::FrameCritical))
		{
			return true;
		}

		graph.Error = State::CreateError(
		    TaskGraphErrorCode::InvalidLaneDependency,
		    "FrameCritical parent and nested task completion must remain in the FrameCritical lane.");
		return false;
	}

	void RejectCycles(
	    TaskGraphStorage& graph,
	    const std::vector<std::vector<std::uint32_t>>& startAdjacency,
	    const std::vector<std::vector<std::uint32_t>>& completionAdjacency) const
	{
		if (!HasCycle(startAdjacency) && !HasCycle(completionAdjacency))
		{
			return;
		}

		graph.Error = State::CreateError(TaskGraphErrorCode::Cycle, "Task graph contains a dependency or nested-completion cycle.");
	}

	const State& m_builder;
};

std::shared_ptr<TaskGraphStorage> TaskGraphBuilder::State::Compile() const
{
	return Compilation(*this).Build();
}
