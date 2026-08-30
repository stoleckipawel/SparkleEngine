#include "PCH.h"

#include "TransformSystemExecution.h"

#include "World/GameWorldState.h"
#include "World/Systems/CameraDerivedStateEvaluationSystem.h"
#include "World/Systems/TransformEvaluationSystem.h"

namespace ECS
{
	TransformSystemExecution::TransformSystemExecution(GameWorldState& state, const StructureFrozenEpoch& epoch) :
	    m_state(state),
	    m_transformQuery(state.m_registry, epoch),
	    m_cameraDerivedQuery(state.m_registry, epoch)
	{
		m_transformQuery.PrepareWriteTraversal();
		m_cameraDerivedQuery.PrepareWriteTraversal();
	}

	std::uint32_t TransformSystemExecution::GetDirtyTransformCount() const noexcept
	{
		return static_cast<std::uint32_t>(m_state.m_systemArena.DirtyTransforms.size());
	}

	bool TransformSystemExecution::RunTransforms(std::uint32_t begin, std::uint32_t end)
	{
		return m_transformQuery
		    .ForEachEntityRange(
		        m_state.m_systemArena.DirtyTransforms,
		        begin,
		        end,
		        [this](std::size_t index, EntityId entity, const LocalTransform& local, WorldTransform& world)
		        {
			        TransformEvaluationSystem::Evaluate(local, world);
			        m_state.m_systemArena.EvaluatedTransforms[index] = entity;
		        })
		    .Succeeded();
	}

	bool TransformSystemExecution::RunCameraDerivedState(std::uint32_t begin, std::uint32_t end)
	{
		return m_cameraDerivedQuery
		    .ForEachEntityRange(
		        m_state.m_systemArena.DirtyTransforms,
		        begin,
		        end,
		        [this](std::size_t index, EntityId entity, const LocalTransform& local, const Camera&, CameraDerivedState& derived)
		        {
			        CameraDerivedStateEvaluationSystem::Evaluate(local, derived);
			        m_state.m_systemArena.CameraDerivedChanges[index] = entity;
		        })
		    .Succeeded();
	}
}
