#pragma once

#include "GameFramework/Public/GameFrameworkAPI.h"

#include <DirectXMath.h>

struct SPARKLE_ENGINE_API PointLightDesc
{
	bool enabled = false;
	DirectX::XMFLOAT3 position{0.0f, 0.0f, 0.0f};
	float intensity = 0.0f;
	DirectX::XMFLOAT3 color{1.0f, 1.0f, 1.0f};
	float radius = 5.0f;
};