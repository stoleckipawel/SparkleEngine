#pragma once

#include "../RendererAPI.h"

#include <DirectXMath.h>

struct SPARKLE_RENDERER_API RectLight
{
	DirectX::XMFLOAT3 position = {0.0f, 0.0f, 0.0f};
	float width = 1.0f;
	DirectX::XMFLOAT3 direction = {0.0f, -1.0f, 0.0f};
	float height = 1.0f;
	DirectX::XMFLOAT3 tangent = {1.0f, 0.0f, 0.0f};
	// Luminance in candela per square meter.
	float luminance = 1.0f;
	DirectX::XMFLOAT3 color = {1.0f, 1.0f, 1.0f};
	bool castShadow = true;
};
