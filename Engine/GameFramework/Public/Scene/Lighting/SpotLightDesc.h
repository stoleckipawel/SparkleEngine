#pragma once

#include "GameFramework/Public/GameFrameworkAPI.h"

#include <DirectXMath.h>

struct SPARKLE_ENGINE_API SpotLightDesc
{
	DirectX::XMFLOAT3 direction = {0.0f, -1.0f, 0.0f};
	float luminousIntensity = 1.0f;
	float range = 0.0f;
	float radius = 0.05f;
	DirectX::XMFLOAT3 distanceAttenuationCoefficients = {0.0f, 0.0f, 1.0f};
	float innerAngleRadians = 0.0f;
	float outerAngleRadians = DirectX::XM_PIDIV4;
	bool castShadow = true;
};
