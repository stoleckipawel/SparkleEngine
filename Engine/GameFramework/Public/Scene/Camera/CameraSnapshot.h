#pragma once

#include "GameFramework/Public/GameFrameworkAPI.h"

#include <DirectXMath.h>

struct SPARKLE_ENGINE_API CameraSnapshot
{
	DirectX::XMFLOAT3 position{0.0f, 0.0f, 0.0f};
	DirectX::XMFLOAT3 direction{0.0f, 0.0f, 1.0f};
	float fovYDegrees = 60.0f;
	float aspectRatio = 16.0f / 9.0f;
	float nearZ = 0.1f;
	float farZ = 1000.0f;
};
