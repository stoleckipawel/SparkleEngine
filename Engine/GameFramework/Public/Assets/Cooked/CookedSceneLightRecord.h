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
		Unknown = 3,
	};

	struct SPARKLE_ENGINE_API CookedSceneLightRecord
	{
		char name[kCookedSceneLightNameCapacity] = {};
		CookedSceneLightKind kind = CookedSceneLightKind::Unknown;
		DirectX::XMFLOAT4X4 worldTransform = MathUtils::IdentityFloat4x4();
		DirectX::XMFLOAT3 direction = {0.0f, -1.0f, 0.0f};
		// Directional lights use lux; point and spot lights use candela.
		float intensity = 1.0f;
		DirectX::XMFLOAT3 color = {1.0f, 1.0f, 1.0f};
		float range = 0.0f;
		float innerConeAngleRadians = 0.0f;
		float outerConeAngleRadians = DirectX::XM_PIDIV4;
		std::uint32_t sourceNodeIndex = kInvalidCookedSceneLightSourceNodeIndex;
		std::uint32_t flags = 0;
	};
}

static_assert(std::is_trivially_copyable_v<Assets::CookedSceneLightRecord>, "CookedSceneLightRecord must stay trivially copyable.");
