#pragma once

#include "GameFramework/Public/GameFrameworkAPI.h"

#include <DirectXMath.h>

struct SPARKLE_ENGINE_API RectLightDesc
{
	DirectX::XMFLOAT3 direction = {0.0f, -1.0f, 0.0f};
	float width = 1.0f;
	DirectX::XMFLOAT3 tangent = {1.0f, 0.0f, 0.0f};
	float height = 1.0f;
	bool castShadow = true;
};
