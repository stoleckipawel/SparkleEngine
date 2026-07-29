#pragma once

#include "Core/Public/Math/MathUtils.h"
#include "GameFramework/Public/GameFrameworkAPI.h"

#include <DirectXMath.h>

#include <string>

struct SPARKLE_ENGINE_API SceneLightCommonDesc
{
	std::string name;
	DirectX::XMFLOAT4X4 worldTransform = MathUtils::IdentityFloat4x4();
	DirectX::XMFLOAT3 color = {1.0f, 1.0f, 1.0f};
	bool visible = true;
};
