#pragma once

#include "GameFramework/Public/Assets/Cooked/CookedAssetCommon.h"
#include "Core/Public/Math/WorldCoordinateSystem.h"

#include <DirectXMath.h>

#include <cstdint>
#include <limits>
#include <type_traits>

namespace Assets
{
	inline constexpr std::uint32_t kCookedAnimationAssetMagic = 0x4D4E4153u;
	inline constexpr std::uint32_t kCookedAnimationAssetVersion = 2;
	inline constexpr std::uint32_t kInvalidCookedAnimationJointIndex = (std::numeric_limits<std::uint32_t>::max)();

	enum class CookedAnimationInterpolation : std::uint32_t
	{
		Linear = 0,
		Step = 1,
		CubicSpline = 2,
	};

	enum class CookedAnimationTargetPath : std::uint32_t
	{
		Translation = 0,
		Rotation = 1,
		Scale = 2,
		Weights = 3,
		Unknown = 0xFFFFFFFFu,
	};

	struct SPARKLE_ENGINE_API CookedAnimationAssetHeader
	{
		CookedAssetHeader fileHeader{kCookedAnimationAssetMagic, kCookedAnimationAssetVersion};
		std::uint32_t coordinateContractVersion = WorldCoordinates::kCoordinateContractVersion;
		char name[64] = {};
		CookedAssetId targetSkeletonAssetId = InvalidCookedAssetId;
		std::uint32_t sourceAnimationIndex = 0;
		float durationSeconds = 0.0f;
		std::uint32_t channelCount = 0;
		std::uint32_t keyframeCount = 0;
		std::uint32_t channelStride = 0;
		std::uint32_t keyframeStride = 0;
		std::uint32_t flags = 0;
	};

	struct SPARKLE_ENGINE_API CookedAnimationChannelRecord
	{
		CookedAnimationTargetPath targetPath = CookedAnimationTargetPath::Unknown;
		CookedAnimationInterpolation interpolation = CookedAnimationInterpolation::Linear;
		std::uint32_t targetNodeIndex = (std::numeric_limits<std::uint32_t>::max)();
		std::uint32_t targetJointIndex = kInvalidCookedAnimationJointIndex;
		std::uint32_t firstKeyframe = 0;
		std::uint32_t keyframeCount = 0;
	};

	struct SPARKLE_ENGINE_API CookedAnimationKeyframeRecord
	{
		float timeSeconds = 0.0f;
		DirectX::XMFLOAT4 value = {};
		DirectX::XMFLOAT4 inTangent = {};
		DirectX::XMFLOAT4 outTangent = {};
	};
}

static_assert(std::is_trivially_copyable_v<Assets::CookedAnimationAssetHeader>, "CookedAnimationAssetHeader must stay trivially copyable.");
static_assert(
    std::is_trivially_copyable_v<Assets::CookedAnimationChannelRecord>,
    "CookedAnimationChannelRecord must stay trivially copyable.");
static_assert(
    std::is_trivially_copyable_v<Assets::CookedAnimationKeyframeRecord>,
    "CookedAnimationKeyframeRecord must stay trivially copyable.");
