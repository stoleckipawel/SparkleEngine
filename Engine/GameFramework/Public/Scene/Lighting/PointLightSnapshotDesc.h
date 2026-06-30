#pragma once

#include "GameFramework/Public/GameFrameworkAPI.h"

#include <DirectXMath.h>

struct SPARKLE_ENGINE_API PointLightSnapshotDesc
{
	DirectX::XMFLOAT3 position = {0.0f, 0.0f, 0.0f};
	float range = 0.0f;
	DirectX::XMFLOAT3 color = {1.0f, 1.0f, 1.0f};
	// Luminous intensity in candela.
	float intensity = 1.0f;
	float sourceRadius = 0.05f;
	bool castShadow = true;
};
