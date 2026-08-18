#pragma once

#include "GameFramework/Public/GameFrameworkAPI.h"

struct SPARKLE_ENGINE_API CameraNavigationSettings final
{
	float MoveSpeedMetersPerSecond = 0.15f;
	float MinimumMoveSpeedMetersPerSecond = 0.0001f;
	float MaximumMoveSpeedMetersPerSecond = 1000.0f;
	float RotationSpeedDegreesPerPixel = 0.0716197f;
	float SprintMultiplier = 2.0f;
	bool InvertY = false;

	bool operator==(const CameraNavigationSettings&) const noexcept = default;
};
