#pragma once

#include "Renderer/Public/RendererAPI.h"

#include <DirectXMath.h>

#include <cstdint>

struct SPARKLE_RENDERER_API PointLight
{
	DirectX::XMFLOAT3 position = {0.0f, 0.0f, 0.0f};
	float intensity = 0.0f;
	DirectX::XMFLOAT3 color = {1.0f, 1.0f, 1.0f};
	float radius = 5.0f;
	std::uint32_t enabled = 0;
	DirectX::XMFLOAT3 padding = {0.0f, 0.0f, 0.0f};
};