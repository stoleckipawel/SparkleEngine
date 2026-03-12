#pragma once

#include "Renderer/Public/RendererAPI.h"

#include <DirectXMath.h>

struct SPARKLE_RENDERER_API DirectionalLight
{
	DirectX::XMFLOAT3 direction = {0.0f, -1.0f, 0.0f};
	float intensity = 1.0f;
	DirectX::XMFLOAT3 color = {1.0f, 1.0f, 1.0f};
	float _padding = 0.0f;
};
