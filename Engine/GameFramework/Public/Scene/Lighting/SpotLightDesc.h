#pragma once

#include "GameFramework/Public/GameFrameworkAPI.h"

#include <DirectXMath.h>

struct SPARKLE_ENGINE_API SpotLightDesc
{
	DirectX::XMFLOAT3 direction = {0.0f, -1.0f, 0.0f};
	float range = 0.0f;
	float sourceRadius = 0.05f;
	float innerConeAngleRadians = 0.0f;
	float outerConeAngleRadians = DirectX::XM_PIDIV4;
	bool castShadow = true;
};
