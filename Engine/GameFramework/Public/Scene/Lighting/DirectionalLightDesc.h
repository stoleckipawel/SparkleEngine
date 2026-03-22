#pragma once

#include "GameFramework/Public/GameFrameworkAPI.h"

#include <DirectXMath.h>

struct SPARKLE_ENGINE_API DirectionalLightDesc
{
	DirectX::XMFLOAT3 direction{0.0f, -1.0f, 0.0f};
	float intensity = 1.0f;
	DirectX::XMFLOAT3 color{1.0f, 1.0f, 1.0f};
};
