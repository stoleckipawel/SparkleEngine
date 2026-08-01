#include "PCH.h"

#include "World/Systems/GameSystemGraph.h"

#include "World/Systems/CompiledGameSystemGraphData.h"
#include "World/Systems/Execution/GameSystemExecution.h"
#include "Tasks/Public/TaskExecutionContext.h"
#include "Tasks/Public/TaskGraph.h"

#include <algorithm>
#include <format>
#include <limits>
#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>

namespace ECS
{
	class GameSystemGraphCompiler final
	{
	  public:
		explicit GameSystemGraphCompiler(const std::vector<GameSystemDesc>& systems) :
		    m_data(std::make_unique<CompiledGameSystemGraphData>())
		{
			m_data->Systems = systems;
			m_data->Edges.resize(systems.size());
		}

		std::unique_ptr<CompiledGameSystemGraphData> Compile()
		{
			if (!ValidateSystemDeclarations())
			{
				return std::move(m_data);
			}
			if (!BuildDeclaredDependencies())
			{
				return std::move(m_data);
			}

			AddPhaseDependencies();
			if (!ValidateHazardOrdering() || !ValidateAcyclicTopology())
			{
				return std::move(m_data);
			}

			BuildTaskGraph();
			return std::move(m_data);
		}

	  private:
		struct ResourcePhaseRange final
		{
			GameSystemPhase First;
			GameSystemPhase Last;
		};

		static constexpr ResourcePhaseRange GetResourcePhaseRange(GameSystemResourceDomain domain) noexcept
		{
			switch (domain)
			{
				case GameSystemResourceDomain::UpdateInputs:
					return {GameSystemPhase::Simulation, GameSystemPhase::Animation};
				case GameSystemResourceDomain::CameraInputIntent:
					return {GameSystemPhase::Simulation, GameSystemPhase::Simulation};
				case GameSystemResourceDomain::MotionClock:
				case GameSystemResourceDomain::SystemChangeScratch:
					return {GameSystemPhase::Simulation, GameSystemPhase::Deformation};
				case GameSystemResourceDomain::AnimationClips:
				case GameSystemResourceDomain::SkeletonResources:
					return {GameSystemPhase::Animation, GameSystemPhase::Extraction};
				case GameSystemResourceDomain::PoseScratch:
				case GameSystemResourceDomain::MorphScratch:
					return {GameSystemPhase::Animation, GameSystemPhase::Deformation};
				case GameSystemResourceDomain::SkinningOutput:
				case GameSystemResourceDomain::MorphOutput:
				case GameSystemResourceDomain::DirtyTransforms:
				case GameSystemResourceDomain::WorldChanges:
					return {GameSystemPhase::Deformation, GameSystemPhase::Extraction};
				case GameSystemResourceDomain::TransformScratch:
				case GameSystemResourceDomain::CameraDerivedScratch:
					return {GameSystemPhase::Transform, GameSystemPhase::Extraction};
				case GameSystemResourceDomain::MeshResources:
				case GameSystemResourceDomain::ExtractionScratch:
				case GameSystemResourceDomain::ExtractionOutput:
				case GameSystemResourceDomain::WorldPublication:
					return {GameSystemPhase::Extraction, GameSystemPhase::Extraction};
			}
			return {GameSystemPhase::Extraction, GameSystemPhase::Simulation};
		}

		static bool IsWrite(ComponentAccessMode mode) noexcept { return mode == ComponentAccessMode::Write; }

		static bool IsWrite(GameSystemAccessMode mode) noexcept { return mode == GameSystemAccessMode::Write; }

		static bool ComponentsConflict(const GameSystemDesc& lhs, const GameSystemDesc& rhs) noexcept
		{
			for (const ComponentAccessDesc& left : lhs.Components)
			{
				for (const ComponentAccessDesc& right : rhs.Components)
				{
					if (left.Type == right.Type && (IsWrite(left.Mode) || IsWrite(right.Mode)))
					{
						return true;
					}
				}
			}
			return false;
		}

		static bool ResourcesConflict(const GameSystemDesc& lhs, const GameSystemDesc& rhs) noexcept
		{
			for (const GameSystemResourceAccess& left : lhs.Resources)
			{
				for (const GameSystemResourceAccess& right : rhs.Resources)
				{
					if (left.Domain == right.Domain && (IsWrite(left.Mode) || IsWrite(right.Mode)))
					{
						return true;
					}
				}
			}
			return false;
		}

		static bool HasPath(const std::vector<std::vector<std::uint32_t>>& edges, std::uint32_t from, std::uint32_t to)
		{
			std::vector<bool> visited(edges.size());
			std::vector<std::uint32_t> stack{from};
			while (!stack.empty())
			{
				const std::uint32_t current = stack.back();
				stack.pop_back();
				if (current == to)
				{
					return true;
				}
				if (visited[current])
				{
					continue;
				}
				visited[current] = true;
				for (std::uint32_t next : edges[current])
				{
					stack.push_back(next);
				}
			}
			return false;
		}

		static void AddEdge(std::vector<std::vector<std::uint32_t>>& edges, std::uint32_t from, std::uint32_t to)
		{
			std::vector<std::uint32_t>& outgoing = edges[from];
			if (std::find(outgoing.begin(), outgoing.end(), to) == outgoing.end())
			{
				outgoing.push_back(to);
			}
		}

		bool ValidateSystemDeclarations()
		{
			for (std::uint32_t index = 0; index < m_data->Systems.size(); ++index)
			{
				const GameSystemDesc& system = m_data->Systems[index];
				if (!system.Id.IsValid())
				{
					m_data->Error = {GameSystemGraphErrorCode::EmptySystemId, "A game system has an empty stable ID."};
					return false;
				}
				if (system.Name.empty())
				{
					m_data->Error = {GameSystemGraphErrorCode::EmptySystemName, "A game system has an empty name."};
					return false;
				}
				if (system.Components.empty() && system.Resources.empty())
				{
					m_data->Error = {
					    GameSystemGraphErrorCode::UndeclaredAccess,
					    std::format("Game system '{}' declares no component query or resource access.", system.Name)};
					return false;
				}
				if (!m_systemById.emplace(system.Id.Value, index).second || !m_systemByName.emplace(system.Name, index).second)
				{
					m_data->Error = {GameSystemGraphErrorCode::DuplicateSystem, std::format("Duplicate game system '{}'.", system.Name)};
					return false;
				}
				if (!ValidateAccessDeclarations(system) || !ValidateExecutionPolicy(system))
				{
					return false;
				}
			}
			return true;
		}

		bool ValidateAccessDeclarations(const GameSystemDesc& system)
		{
			for (std::size_t left = 0; left < system.Components.size(); ++left)
			{
				for (std::size_t right = left + 1; right < system.Components.size(); ++right)
				{
					if (system.Components[left].Type == system.Components[right].Type)
					{
						m_data->Error = {
						    system.Components[left].Mode == system.Components[right].Mode
						        ? GameSystemGraphErrorCode::DuplicateAccess
						        : GameSystemGraphErrorCode::ConflictingAccessDeclaration,
						    std::format("Game system '{}' declares one component domain more than once.", system.Name)};
						return false;
					}
				}
			}

			for (std::size_t left = 0; left < system.Resources.size(); ++left)
			{
				const ResourcePhaseRange range = GetResourcePhaseRange(system.Resources[left].Domain);
				if (system.Phase < range.First || system.Phase > range.Last)
				{
					m_data->Error = {
					    GameSystemGraphErrorCode::UnavailablePhaseResource,
					    std::format("Game system '{}' declares a resource unavailable in its phase.", system.Name)};
					return false;
				}
				for (std::size_t right = left + 1; right < system.Resources.size(); ++right)
				{
					if (system.Resources[left].Domain == system.Resources[right].Domain)
					{
						m_data->Error = {
						    system.Resources[left].Mode == system.Resources[right].Mode
						        ? GameSystemGraphErrorCode::DuplicateAccess
						        : GameSystemGraphErrorCode::ConflictingAccessDeclaration,
						    std::format("Game system '{}' declares one resource domain more than once.", system.Name)};
						return false;
					}
				}
			}
			return true;
		}

		bool ValidateExecutionPolicy(const GameSystemDesc& system)
		{
			if (system.Execution.Mode == GameSystemExecutionMode::ParallelRanges &&
			    (system.Execution.RangePolicy.GrainSize == 0 || system.Execution.RangePolicy.MaximumPartitions == 0))
			{
				m_data->Error = {
				    GameSystemGraphErrorCode::TaskGraphRejected,
				    std::format("Game system '{}' has an invalid grain policy.", system.Name)};
				return false;
			}
			return true;
		}

		bool BuildDeclaredDependencies()
		{
			for (std::uint32_t index = 0; index < m_data->Systems.size(); ++index)
			{
				const GameSystemDesc& system = m_data->Systems[index];
				for (GameSystemId prerequisiteId : system.Prerequisites)
				{
					const auto prerequisite = m_systemById.find(prerequisiteId.Value);
					if (prerequisite == m_systemById.end())
					{
						m_data->Error = {
						    GameSystemGraphErrorCode::MissingPrerequisite,
						    std::format("Game system '{}' has a missing prerequisite.", system.Name)};
						return false;
					}
					if (m_data->Systems[prerequisite->second].Phase > system.Phase)
					{
						m_data->Error = {
						    GameSystemGraphErrorCode::InvalidPhaseDependency,
						    std::format("Game system '{}' depends on a later phase.", system.Name)};
						return false;
					}
					AddEdge(m_data->Edges, prerequisite->second, index);
				}
			}
			return true;
		}

		void AddPhaseDependencies()
		{
			for (std::uint32_t left = 0; left < m_data->Systems.size(); ++left)
			{
				for (std::uint32_t right = left + 1; right < m_data->Systems.size(); ++right)
				{
					const GameSystemDesc& lhs = m_data->Systems[left];
					const GameSystemDesc& rhs = m_data->Systems[right];
					if (lhs.Phase < rhs.Phase)
					{
						AddEdge(m_data->Edges, left, right);
					}
					else if (rhs.Phase < lhs.Phase)
					{
						AddEdge(m_data->Edges, right, left);
					}
				}
			}
		}

		bool ValidateHazardOrdering()
		{
			for (std::uint32_t left = 0; left < m_data->Systems.size(); ++left)
			{
				for (std::uint32_t right = left + 1; right < m_data->Systems.size(); ++right)
				{
					const GameSystemDesc& lhs = m_data->Systems[left];
					const GameSystemDesc& rhs = m_data->Systems[right];
					if (lhs.Phase != rhs.Phase || (!ComponentsConflict(lhs, rhs) && !ResourcesConflict(lhs, rhs)))
					{
						continue;
					}
					if (!HasPath(m_data->Edges, left, right) && !HasPath(m_data->Edges, right, left))
					{
						m_data->Error = {
						    GameSystemGraphErrorCode::AmbiguousHazard,
						    std::format("Game systems '{}' and '{}' have an unordered write hazard.", lhs.Name, rhs.Name)};
						return false;
					}
				}
			}
			return true;
		}

		bool ValidateAcyclicTopology()
		{
			for (std::uint32_t from = 0; from < m_data->Edges.size(); ++from)
			{
				for (std::uint32_t to : m_data->Edges[from])
				{
					if (HasPath(m_data->Edges, to, from))
					{
						m_data->Error = {GameSystemGraphErrorCode::Cycle, "Game-system prerequisites contain a cycle."};
						return false;
					}
				}
			}
			return true;
		}

		void BuildTaskGraph()
		{
			std::uint32_t taskCount = 0;
			for (const GameSystemDesc& system : m_data->Systems)
			{
				taskCount += system.Execution.Mode == GameSystemExecutionMode::ParallelRanges
				                 ? 1u + system.Execution.RangePolicy.MaximumPartitions
				                 : 1u;
			}

			std::uint32_t edgeCount = 0;
			for (const auto& outgoing : m_data->Edges)
			{
				edgeCount += static_cast<std::uint32_t>(outgoing.size());
			}

			TaskGraphBuilder tasks(
			    TaskGraphLimits{.MaximumTasks = (std::max) (taskCount, 1u), .MaximumEdges = (std::max) (edgeCount + taskCount, 1u)});
			std::vector<TaskNodeHandle> systemNodes;
			systemNodes.reserve(m_data->Systems.size());
			for (std::uint32_t systemIndex = 0; systemIndex < m_data->Systems.size(); ++systemIndex)
			{
				AddSystemTasks(tasks, systemNodes, systemIndex);
			}
			for (std::uint32_t from = 0; from < m_data->Edges.size(); ++from)
			{
				for (std::uint32_t to : m_data->Edges[from])
				{
					tasks.DependsOn(systemNodes[to], systemNodes[from]);
				}
			}

			m_data->Tasks = tasks.Compile();
			if (!m_data->Tasks)
			{
				m_data->Error = {GameSystemGraphErrorCode::TaskGraphRejected, m_data->Tasks.GetError().Message};
			}
		}

		void AddSystemTasks(TaskGraphBuilder& tasks, std::vector<TaskNodeHandle>& systemNodes, std::uint32_t systemIndex)
		{
			const GameSystemDesc& system = m_data->Systems[systemIndex];
			const TaskDesc descriptor{TaskName(system.Name), TaskLane::FrameCritical};
			if (system.Execution.Mode == GameSystemExecutionMode::SingleTask)
			{
				systemNodes.push_back(tasks.Add(
				    descriptor,
				    [systemIndex](TaskExecutionContext& context)
				    {
					    return GameSystemExecution::ExecutePartition(
					        systemIndex,
					        0,
					        ParallelForPolicy{1, (std::numeric_limits<std::uint32_t>::max)(), 1},
					        context);
				    }));
				return;
			}

			const TaskNodeHandle group = tasks.Add(
			    descriptor,
			    [](TaskExecutionContext&)
			    {
				    return TaskResult::Success();
			    });
			systemNodes.push_back(group);
			for (std::uint32_t partition = 0; partition < system.Execution.RangePolicy.MaximumPartitions; ++partition)
			{
				const std::string taskName = std::format("{}.Range{}", system.Name, partition);
				tasks.AddNested(
				    group,
				    TaskDesc{TaskName(taskName), TaskLane::FrameCritical},
				    [systemIndex, partition, policy = system.Execution.RangePolicy](TaskExecutionContext& context)
				    {
					    return GameSystemExecution::ExecutePartition(systemIndex, partition, policy, context);
				    });
			}
		}

		std::unique_ptr<CompiledGameSystemGraphData> m_data;
		std::unordered_map<std::uint64_t, std::uint32_t> m_systemById;
		std::unordered_map<std::string, std::uint32_t> m_systemByName;
	};

	CompiledGameSystemGraph GameSystemGraph::Compile() const
	{
		GameSystemGraphCompiler compiler(m_systems);
		return CompiledGameSystemGraph(compiler.Compile());
	}
}
