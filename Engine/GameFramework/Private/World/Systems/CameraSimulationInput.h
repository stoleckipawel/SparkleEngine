#pragma once

#include "GameFramework/Public/Scene/Camera/CameraInputIntent.h"
#include "GameFramework/Public/Scene/Camera/CameraNavigationSettings.h"

namespace ECS
{
	struct CameraSimulationInput final
	{
		const CameraInputIntent& Intent;
		const CameraNavigationSettings& NavigationSettings;
		float DeltaSeconds = 0.0f;
	};
}
