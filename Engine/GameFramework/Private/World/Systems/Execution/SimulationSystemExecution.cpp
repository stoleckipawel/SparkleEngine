#include "PCH.h"

#include "SimulationSystemExecution.h"

#include "World/GameWorldState.h"
namespace ECS
{
	SimulationSystemExecution::SimulationSystemExecution(
	    GameWorldState& state,
	    const CameraSimulationInput& cameraInput,
	    const StructureFrozenEpoch& epoch) :
	    m_state(state),
	    m_cameraMovement(state.m_activeCamera, cameraInput),
	    m_cameraQuery(state.m_registry, epoch)
	{
		m_cameraQuery.PrepareWriteTraversal();
	}

	std::uint32_t SimulationSystemExecution::GetCameraCount() const noexcept
	{
		return static_cast<std::uint32_t>(m_cameraQuery.GetEstimatedEntityCount());
	}

	bool SimulationSystemExecution::RunCamera(std::uint32_t begin, std::uint32_t end)
	{
		return m_cameraQuery
		    .ForEachRange(
		        begin,
		        end,
		        [this](std::size_t index, EntityId entity, Camera& camera, LocalTransform& transform)
		        {
			        if (m_cameraMovement.Apply(entity, camera, transform))
			        {
				        m_state.m_systemArena.CameraChanges[index] = entity;
			        }
		        })
		    .Succeeded();
	}
}
