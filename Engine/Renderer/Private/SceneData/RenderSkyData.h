#pragma once

#include <DirectXMath.h>

class Texture;

struct RenderSkyData final
{
	const Texture* skyTexture = nullptr;
	DirectX::XMFLOAT3 color{1.0f, 1.0f, 1.0f};
	float intensity = 1.0f;
	bool enabled = true;
};
