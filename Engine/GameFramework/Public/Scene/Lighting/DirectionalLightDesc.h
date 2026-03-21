#pragma once

#include "Core/Public/Math/MathUtils.h"
#include "GameFramework/Public/GameFrameworkAPI.h"

#include <algorithm>
#include <DirectXMath.h>

struct SPARKLE_ENGINE_API DirectionalLightDesc
{
	static DirectionalLightDesc Sanitize(const DirectionalLightDesc& directionalLight) noexcept
	{
		DirectionalLightDesc sanitized = directionalLight;
		sanitized.direction = MathUtils::Normalize3(sanitized.direction, {0.0f, -1.0f, 0.0f});
		sanitized.intensity = (std::max) (0.0f, sanitized.intensity);
		sanitized.color.x = std::clamp(sanitized.color.x, 0.0f, 1.0f);
		sanitized.color.y = std::clamp(sanitized.color.y, 0.0f, 1.0f);
		sanitized.color.z = std::clamp(sanitized.color.z, 0.0f, 1.0f);
		return sanitized;
	}

	static DirectionalLightDesc FromRuntimeState(
	    const DirectX::XMFLOAT3& direction,
	    float intensity,
	    const DirectX::XMFLOAT3& color) noexcept
	{
		DirectionalLightDesc lightDesc;
		lightDesc.direction = direction;
		lightDesc.intensity = intensity;
		lightDesc.color = color;
		return Sanitize(lightDesc);
	}

	DirectX::XMFLOAT3 direction{0.0f, -1.0f, 0.0f};
	float intensity = 1.0f;
	DirectX::XMFLOAT3 color{1.0f, 1.0f, 1.0f};
};