#pragma once

#include "GameFramework/Public/GameFrameworkAPI.h"

#include <DirectXMath.h>

struct SPARKLE_ENGINE_API SpotLightSnapshotDesc
{
	DirectX::XMFLOAT3 position = {0.0f, 0.0f, 0.0f};
	float range = 0.0f;
	DirectX::XMFLOAT3 direction = {0.0f, -1.0f, 0.0f};
	float innerConeAngleRadians = 0.0f;
	DirectX::XMFLOAT3 color = {1.0f, 1.0f, 1.0f};
	float intensity = 1.0f;
	float outerConeAngleRadians = 0.0f;
	bool castShadow = true;
	DirectX::XMUINT2 padding = {0u, 0u};
};
