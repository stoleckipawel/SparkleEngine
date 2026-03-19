#pragma once

#include "GameFramework/Public/GameFrameworkAPI.h"

#include <DirectXMath.h>

struct SPARKLE_ENGINE_API DirectionalLightDesc
{
	static DirectionalLightDesc FromRuntimeState(
	    const DirectX::XMFLOAT3& direction,
	    float intensity,
	    const DirectX::XMFLOAT3& color) noexcept
	{
		DirectionalLightDesc lightDesc;
		lightDesc.direction = direction;
		lightDesc.intensity = intensity;
		lightDesc.color = color;
		return lightDesc;
	}

	DirectX::XMFLOAT3 direction{0.0f, -1.0f, 0.0f};
	float intensity = 1.0f;
	DirectX::XMFLOAT3 color{1.0f, 1.0f, 1.0f};
};