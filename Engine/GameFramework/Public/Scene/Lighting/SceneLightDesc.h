#pragma once

#include "GameFramework/Public/GameFrameworkAPI.h"
#include "GameFramework/Public/Scene/Lighting/DirectionalLightDesc.h"

#include <DirectXMath.h>

#include <cstdint>
#include <string>

enum class SceneLightKind : std::uint32_t
{
	Directional = 0,
	Point = 1,
	Spot = 2,
	Unknown = 3,
};

struct SPARKLE_ENGINE_API SceneLightDesc
{
	std::string name;
	SceneLightKind kind = SceneLightKind::Unknown;
	DirectionalLightDesc directional;
	DirectX::XMFLOAT4X4 worldTransform = {};
	float range = 0.0f;
	float innerConeAngleRadians = 0.0f;
	float outerConeAngleRadians = 0.0f;
	bool visible = true;

	bool IsDirectional() const noexcept { return kind == SceneLightKind::Directional; }
};
