#pragma once

#include "World/Systems/CameraMovementSystem.h"
#include "World/Systems/Descriptors/GameWorldSystemContract.h"

namespace ECS
{
	class GameWorldState;

	class SimulationSystemExecution final
	{
	public:
		SimulationSystemExecution(GameWorldState& state, const CameraSimulationInput& cameraInput, const StructureFrozenEpoch& epoch);

		std::uint32_t GetCameraCount() const noexcept;
		bool RunCamera(std::uint32_t begin, std::uint32_t end);

	private:
		GameWorldState& m_state;
		CameraMovementSystem m_cameraMovement;
		CameraMovementQuery m_cameraQuery;
	};
}
