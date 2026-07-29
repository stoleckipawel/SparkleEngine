#pragma once

#include "GameFramework/Public/Assets/Cooked/CookedTextureReference.h"
#include "GameFramework/Public/GameFrameworkAPI.h"

#include <DirectXMath.h>

struct SPARKLE_ENGINE_API SceneSkyDesc
{
	bool enabled = true;
	DirectX::XMFLOAT3 color{1.0f, 1.0f, 1.0f};
	float brightness = 1.0f;
	Assets::CookedTextureReference skyTexture{{}, TextureGroup::HdrColor};
};
