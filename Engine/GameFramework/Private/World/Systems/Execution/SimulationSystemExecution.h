#pragma once

#include "GameFramework/Public/Scene/Camera/CameraInputIntent.h"
#include "World/Systems/Descriptors/GameWorldSystemContract.h"

namespace ECS
{
	class GameWorldState;

	class SimulationSystemExecution final
	{
	  public:
		SimulationSystemExecution(
		    GameWorldState& state,
		    const CameraInputIntent& cameraIntent,
		    float deltaSeconds,
		    const StructureFrozenEpoch& epoch);

		std::uint32_t GetCameraCount() const noexcept;
		std::uint32_t GetMotionCount() const noexcept;
		float GetNextMotionTime() const noexcept { return m_nextMotionTime; }
		bool RunCamera(std::uint32_t begin, std::uint32_t end);
		bool RunMotion(std::uint32_t begin, std::uint32_t end);

	  private:
		GameWorldState& m_state;
		const CameraInputIntent& m_cameraIntent;
		float m_deltaSeconds = 0.0f;
		float m_nextMotionTime = 0.0f;
		CameraMovementQuery m_cameraQuery;
		OscillatingMotionQuery m_motionQuery;
	};
}
