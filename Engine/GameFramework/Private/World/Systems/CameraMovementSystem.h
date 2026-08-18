#pragma once

#include "GameFramework/Public/World/EntityId.h"
#include "World/Systems/CameraSimulationInput.h"

namespace ECS
{
	struct Camera;
	struct LocalTransform;

	class CameraMovementSystem final
	{
	public:
		CameraMovementSystem(EntityId activeCamera, const CameraSimulationInput& input) noexcept;

		bool Apply(EntityId entity, Camera& camera, LocalTransform& transform) const noexcept;

	private:
		EntityId m_activeCamera;
		const CameraInputIntent& m_intent;
		const CameraNavigationSettings& m_navigationSettings;
		float m_deltaSeconds = 0.0f;
	};
}
