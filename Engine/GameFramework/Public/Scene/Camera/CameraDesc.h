#pragma once

#include "GameFramework/Public/GameFrameworkAPI.h"

#include <DirectXMath.h>

struct SPARKLE_ENGINE_API CameraDesc
{
	DirectX::XMFLOAT3 position{0.0f, 0.0f, -4.0f};
	float yawRadians = 0.0f;
	float pitchRadians = 0.0f;
	float fovYDegrees = 60.0f;
	float moveSpeed = 0.15f;
};