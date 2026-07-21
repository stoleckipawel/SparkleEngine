#include "PCH.h"

#include "SimulationSystemExecution.h"

#include "World/GameWorldState.h"
#include "World/Systems/CameraMovementSystem.h"
#include "World/Systems/OscillatingMeshMotionSystem.h"

#include <algorithm>

namespace ECS
{
	SimulationSystemExecution::SimulationSystemExecution(
	    GameWorldState& state,
	    const CameraInputIntent& cameraIntent,
	    float deltaSeconds,
	    const StructureFrozenEpoch& epoch) :
	    m_state(state),
	    m_cameraIntent(cameraIntent),
	    m_deltaSeconds((std::max)(0.0f, deltaSeconds)),
	    m_nextMotionTime(state.m_motionTimeSeconds + m_deltaSeconds),
	    m_cameraQuery(state.m_registry, epoch),
	    m_motionQuery(state.m_registry, epoch)
	{
		m_cameraQuery.PrepareWriteTraversal();
		m_motionQuery.PrepareWriteTraversal();
	}

	std::uint32_t SimulationSystemExecution::GetCameraCount() const noexcept
	{
		return static_cast<std::uint32_t>(m_cameraQuery.GetEstimatedEntityCount());
	}

	std::uint32_t SimulationSystemExecution::GetMotionCount() const noexcept
	{
		return static_cast<std::uint32_t>(m_motionQuery.GetEstimatedEntityCount());
	}

	bool SimulationSystemExecution::RunCamera(std::uint32_t begin, std::uint32_t end)
	{
		return m_cameraQuery.ForEachRange(
		                         begin,
		                         end,
		                         [this](std::size_t index, EntityId entity, Camera& camera, CameraMovement& movement, LocalTransform& transform)
		                         {
			                         if (CameraMovementSystem::Apply(
			                                 entity,
			                                 m_state.m_activeCamera,
			                                 m_cameraIntent,
			                                 m_deltaSeconds,
			                                 camera,
			                                 movement,
			                                 transform))
			                         {
				                         m_state.m_systemArena.CameraChanges[index] = entity;
			                         }
		                         })
		    .Succeeded();
	}

	bool SimulationSystemExecution::RunMotion(std::uint32_t begin, std::uint32_t end)
	{
		const bool useLanes = m_motionQuery.GetEstimatedEntityCount() > 1;
		return m_motionQuery.ForEachRange(
		                         begin,
		                         end,
		                         [this, useLanes](std::size_t index, EntityId entity, const OscillatingMotion& motion, LocalTransform& transform)
		                         {
			                         OscillatingMeshMotionSystem::Apply(motion, m_nextMotionTime, useLanes, transform);
			                         m_state.m_systemArena.MotionChanges[index] = entity;
		                         })
		    .Succeeded();
	}
}
