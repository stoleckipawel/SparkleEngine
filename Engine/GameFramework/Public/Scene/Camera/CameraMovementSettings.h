#pragma once

#include "GameFramework/Public/GameFrameworkAPI.h"

struct SPARKLE_ENGINE_API CameraMovementSettings
{
	float moveSpeedMetersPerSecond = 0.10f;
	float minMoveSpeedMetersPerSecond = 0.0001f;
	float maxMoveSpeedMetersPerSecond = 10.0f;
	float speedStepMetersPerSecond = 0.0001f;
	float sprintMultiplier = 2.0f;
	float mouseSensitivity = 0.00125f;
	bool invertY = false;
};
