#pragma once

#include "Core/Public/Math/MathUtils.h"

#include <DirectXMath.h>

#include <cstdint>
#include <limits>
#include <string>

enum class ImportedLightKind : std::uint32_t
{
	Directional = 0,
	Point = 1,
	Spot = 2,
	Unknown = 3,
};

struct ImportedLight
{
	std::string name;
	ImportedLightKind kind = ImportedLightKind::Unknown;
	DirectX::XMFLOAT4X4 worldTransform = MathUtils::IdentityFloat4x4();
	DirectX::XMFLOAT3 direction = {0.0f, -1.0f, 0.0f};
	DirectX::XMFLOAT3 color = {1.0f, 1.0f, 1.0f};
	// Directional lights use lux; point and spot lights use candela.
	float intensity = 1.0f;
	float range = 0.0f;
	float innerConeAngleRadians = 0.0f;
	float outerConeAngleRadians = DirectX::XM_PIDIV4;
	bool visible = true;
	std::uint32_t sourceNodeIndex = (std::numeric_limits<std::uint32_t>::max)();

	bool IsDirectional() const noexcept { return kind == ImportedLightKind::Directional; }
	bool IsPoint() const noexcept { return kind == ImportedLightKind::Point; }
	bool IsSpot() const noexcept { return kind == ImportedLightKind::Spot; }
};
