#pragma once

#include "../RendererAPI.h"

#include <DirectXMath.h>

struct SPARKLE_RENDERER_API DirectionalLight
{
	DirectX::XMFLOAT3 direction = {0.0f, -1.0f, 0.0f};
	// Illuminance in lux.
	float illuminance = 1.0f;
	DirectX::XMFLOAT3 color = {1.0f, 1.0f, 1.0f};
	float angularSizeRadians = 0.009308f;
	bool castShadow = true;
};
