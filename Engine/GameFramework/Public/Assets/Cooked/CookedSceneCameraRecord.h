#pragma once

#include "Core/Public/Math/MathUtils.h"
#include "GameFramework/Public/Assets/Cooked/CookedAssetCommon.h"

#include <DirectXMath.h>

#include <cstdint>
#include <type_traits>

namespace Assets
{
	inline constexpr std::uint32_t kCookedSceneCameraNameCapacity = 64;

	enum class CookedSceneCameraProjectionKind : std::uint32_t
	{
		Perspective = 0,
		Orthographic = 1,
		Unknown = 2,
	};

	struct SPARKLE_ENGINE_API CookedSceneCameraRecord
	{
		char name[kCookedSceneCameraNameCapacity] = {};
		DirectX::XMFLOAT4X4 worldTransform = MathUtils::IdentityFloat4x4();
		CookedSceneCameraProjectionKind projectionKind = CookedSceneCameraProjectionKind::Unknown;
		float fovYRadians = 0.0f;
		float nearZ = 0.1f;
		float farZ = 1000.0f;
		std::uint32_t sourceNodeIndex = 0;
		std::uint32_t flags = 0;
	};
}

static_assert(std::is_trivially_copyable_v<Assets::CookedSceneCameraRecord>, "CookedSceneCameraRecord must stay trivially copyable.");
