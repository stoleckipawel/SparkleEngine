#pragma once

#include "GameFramework/Public/GameFrameworkAPI.h"

struct SPARKLE_ENGINE_API CameraMovementSettings
{
	float moveSpeed = 0.10f;
	float minMoveSpeed = 0.0001f;
	float maxMoveSpeed = 10.0f;
	float speedStep = 0.0001f;
	float sprintMultiplier = 2.0f;
	float mouseSensitivity = 0.00125f;
	bool invertY = false;
};
