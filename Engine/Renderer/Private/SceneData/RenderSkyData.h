#pragma once

#include <DirectXMath.h>

struct RendererTexture;

struct RenderSkyData final
{
	const RendererTexture* texture = nullptr;
	DirectX::XMFLOAT3 color{1.0f, 1.0f, 1.0f};
	float brightness = 1.0f;
	bool enabled = true;

	bool HasTexture() const noexcept { return texture != nullptr; }
};
