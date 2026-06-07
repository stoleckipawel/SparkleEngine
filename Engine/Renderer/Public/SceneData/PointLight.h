#pragma once

#include "../RendererAPI.h"

#include <DirectXMath.h>

struct SPARKLE_RENDERER_API PointLight
{
	DirectX::XMFLOAT3 position = {0.0f, 0.0f, 0.0f};
	float range = 0.0f;
	DirectX::XMFLOAT3 color = {1.0f, 1.0f, 1.0f};
	float intensity = 1.0f;
};
