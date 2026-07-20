#pragma once

#include "GameFramework/Public/GameFrameworkAPI.h"

struct SPARKLE_ENGINE_API CameraInputIntent final
{
	float ForwardAxis = 0.0f;
	float RightAxis = 0.0f;
	float UpAxis = 0.0f;
	float LookDeltaX = 0.0f;
	float LookDeltaY = 0.0f;
	float SpeedStepCount = 0.0f;
	float AspectRatio = 1.0f;
	bool Sprint = false;
	bool HasAspectRatio = false;
};
