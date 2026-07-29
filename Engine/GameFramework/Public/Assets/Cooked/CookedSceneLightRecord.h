#pragma once

#include "Core/Public/Math/MathUtils.h"
#include "GameFramework/Public/GameFrameworkAPI.h"

#include <DirectXMath.h>

#include <cstdint>
#include <limits>
#include <type_traits>

namespace Assets
{
	inline constexpr std::uint32_t kCookedSceneLightNameCapacity = 64;
	inline constexpr std::uint32_t kInvalidCookedSceneLightSourceNodeIndex = (std::numeric_limits<std::uint32_t>::max)();

	enum class CookedSceneLightKind : std::uint32_t
	{
		Directional = 0,
		Point = 1,
		Spot = 2,
		Rect = 3,
		Unknown = 4,
	};

	struct SPARKLE_ENGINE_API CookedSceneLightRecord
	{
		char name[kCookedSceneLightNameCapacity] = {};
		CookedSceneLightKind kind = CookedSceneLightKind::Unknown;
		DirectX::XMFLOAT4X4 worldTransform = MathUtils::IdentityFloat4x4();
		DirectX::XMFLOAT3 direction = {0.0f, -1.0f, 0.0f};
		DirectX::XMFLOAT3 color = {1.0f, 1.0f, 1.0f};
		float illuminance = 1.0f;
		float luminousIntensity = 1.0f;
		float luminance = 1.0f;
		float range = 0.0f;
		DirectX::XMFLOAT3 distanceAttenuationCoefficients = {0.0f, 0.0f, 1.0f};
		float radius = 0.05f;
		float innerAngleRadians = 0.0f;
		float outerAngleRadians = DirectX::XM_PIDIV4;
		float angularSizeRadians = 0.009308f;
		DirectX::XMFLOAT3 tangent = {1.0f, 0.0f, 0.0f};
		float width = 1.0f;
		float height = 1.0f;
		std::uint32_t sourceNodeIndex = kInvalidCookedSceneLightSourceNodeIndex;
		std::uint32_t flags = 0;
	};
}

static_assert(std::is_trivially_copyable_v<Assets::CookedSceneLightRecord>, "CookedSceneLightRecord must stay trivially copyable.");
