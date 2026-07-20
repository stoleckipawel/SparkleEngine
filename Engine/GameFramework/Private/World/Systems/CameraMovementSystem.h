#pragma once

#include "GameFramework/Public/Scene/Camera/CameraInputIntent.h"
#include "GameFramework/Public/World/EntityId.h"

namespace ECS
{
	struct Camera;
	struct CameraMovement;
	struct LocalTransform;

	class CameraMovementSystem final
	{
	  public:
		static bool Apply(
		    EntityId entity,
		    EntityId activeCamera,
		    const CameraInputIntent& intent,
		    float deltaSeconds,
		    Camera& camera,
		    CameraMovement& movement,
		    LocalTransform& transform) noexcept;
	};
}
