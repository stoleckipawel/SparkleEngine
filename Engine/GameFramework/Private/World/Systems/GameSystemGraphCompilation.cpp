#include "PCH.h"

#include "World/Systems/GameSystemGraph.h"

#include "World/Systems/CompiledGameSystemGraphData.h"

#include <algorithm>
#include <format>
#include <limits>
#include <memory>
#include <utility>
#include <vector>

namespace ECS
{
	class GameSystemGraphCompiler final
	{
	public:
		explicit GameSystemGraphCompiler(std::vector<GameSystemDesc> systems) :
		    m_data(std::make_unique<CompiledGameSystemGraphData>()),
		    m_edges(systems.size())
		{
			m_data->Systems = std::move(systems);
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

			BuildExecutionWaves();
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
				case GameSystemResourceDomain::CameraNavigationSettings:
					return {GameSystemPhase::Simulation, GameSystemPhase::Simulation};

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
			if (m_data->Systems.empty())
			{
				m_data->Error = {GameSystemGraphErrorCode::TaskGraphRejected, "Game-system graph has no declared systems."};
				return false;
			}
			for (std::uint32_t index = 0; index < m_data->Systems.size(); ++index)
			{
				const GameSystemDesc& system = m_data->Systems[index];
				if (system.Name.empty())
				{
					m_data->Error = {GameSystemGraphErrorCode::EmptySystemName, "A game system has an empty name."};
					return false;
				}
				if (system.Components.empty() && system.Resources.empty())
				{
					m_data->Error = {GameSystemGraphErrorCode::UndeclaredAccess, std::format("Game system '{}' declares no component query or resource access.", system.Name)};
					return false;
				}
				for (std::uint32_t prior = 0; prior < index; ++prior)
				{
					if (m_data->Systems[prior].Name == system.Name)
					{
						m_data->Error = {GameSystemGraphErrorCode::DuplicateSystem, std::format("Duplicate game system '{}'.", system.Name)};
						return false;
					}
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
						    system.Components[left].Mode == system.Components[right].Mode ? GameSystemGraphErrorCode::DuplicateAccess : GameSystemGraphErrorCode::ConflictingAccessDeclaration,
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
					m_data->Error = {GameSystemGraphErrorCode::UnavailablePhaseResource, std::format("Game system '{}' declares a resource unavailable in its phase.", system.Name)};
					return false;
				}
				for (std::size_t right = left + 1; right < system.Resources.size(); ++right)
				{
					if (system.Resources[left].Domain == system.Resources[right].Domain)
					{
						m_data->Error = {
						    system.Resources[left].Mode == system.Resources[right].Mode ? GameSystemGraphErrorCode::DuplicateAccess : GameSystemGraphErrorCode::ConflictingAccessDeclaration,
						    std::format("Game system '{}' declares one resource domain more than once.", system.Name)};

						return false;
					}
				}
			}
			return true;
		}

		bool ValidateExecutionPolicy(const GameSystemDesc& system)
		{
			if (system.RangePolicy.GrainSize == 0 || system.RangePolicy.MaximumPartitions == 0 || !system.GetItemCount || !system.ExecuteRange)
			{
				m_data->Error = {GameSystemGraphErrorCode::TaskGraphRejected, std::format("Game system '{}' has an invalid range policy or missing execution functions.", system.Name)};
				return false;
			}
			return true;
		}

		bool BuildDeclaredDependencies()
		{
			for (std::uint32_t index = 0; index < m_data->Systems.size(); ++index)
			{
				const GameSystemDesc& system = m_data->Systems[index];
				for (std::uint32_t prerequisite : system.Prerequisites)
				{
					if (prerequisite >= m_data->Systems.size())
					{
						m_data->Error = {GameSystemGraphErrorCode::MissingPrerequisite, std::format("Game system '{}' has a missing prerequisite.", system.Name)};
						return false;
					}
					if (m_data->Systems[prerequisite].Phase > system.Phase)
					{
						m_data->Error = {GameSystemGraphErrorCode::InvalidPhaseDependency, std::format("Game system '{}' depends on a later phase.", system.Name)};
						return false;
					}
					AddEdge(m_edges, prerequisite, index);
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
						AddEdge(m_edges, left, right);
					}
					else if (rhs.Phase < lhs.Phase)
					{
						AddEdge(m_edges, right, left);
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
					if (!HasPath(m_edges, left, right) && !HasPath(m_edges, right, left))
					{
						m_data->Error = {GameSystemGraphErrorCode::AmbiguousHazard, std::format("Game systems '{}' and '{}' have an unordered write hazard.", lhs.Name, rhs.Name)};
						return false;
					}
				}
			}
			return true;
		}

		bool ValidateAcyclicTopology()
		{
			for (std::uint32_t from = 0; from < m_edges.size(); ++from)
			{
				for (std::uint32_t to : m_edges[from])
				{
					if (HasPath(m_edges, to, from))
					{
						m_data->Error = {GameSystemGraphErrorCode::Cycle, "Game-system prerequisites contain a cycle."};
						return false;
					}
				}
			}
			return true;
		}

		void BuildExecutionWaves()
		{
			std::vector<std::uint32_t> remaining(m_data->Systems.size());
			for (const auto& outgoing : m_edges)
				for (std::uint32_t target : outgoing)
					++remaining[target];
			std::vector<bool> scheduled(m_data->Systems.size());
			std::size_t count = 0;
			while (count < scheduled.size())
			{
				std::vector<std::uint32_t> wave;
				for (std::uint32_t index = 0; index < scheduled.size(); ++index)
					if (!scheduled[index] && remaining[index] == 0)
						wave.push_back(index);
				for (std::uint32_t index : wave)
				{
					scheduled[index] = true;
					++count;
					for (std::uint32_t target : m_edges[index])
						--remaining[target];
				}
				GameSystemWave compiledWave;
				compiledWave.ItemCounts.resize(wave.size());
				compiledWave.Systems = std::move(wave);
				m_data->Waves.push_back(std::move(compiledWave));
			}
		}

		std::unique_ptr<CompiledGameSystemGraphData> m_data;
		std::vector<std::vector<std::uint32_t>> m_edges;
	};

	CompiledGameSystemGraph GameSystemGraph::Compile()
	{
		GameSystemGraphCompiler compiler(std::move(m_systems));
		return CompiledGameSystemGraph(compiler.Compile());
	}
}
