#pragma once

#include "GameFramework/Public/GameFrameworkAPI.h"

#include <DirectXMath.h>

struct SPARKLE_ENGINE_API SceneDirectionalLightDesc
{
	DirectX::XMFLOAT3 direction = {0.0f, -1.0f, 0.0f};
	bool castShadow = true;
};
