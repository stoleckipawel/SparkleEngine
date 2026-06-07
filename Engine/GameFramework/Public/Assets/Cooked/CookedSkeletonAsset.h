#pragma once

#include "GameFramework/Public/Assets/Cooked/CookedAssetCommon.h"

#include <DirectXMath.h>

#include <cstdint>
#include <limits>
#include <type_traits>

namespace Assets
{
	inline constexpr std::uint32_t kCookedSkeletonAssetMagic = MakeCookedAssetMagic('S', 'S', 'K', 'L');
	inline constexpr std::uint32_t kCookedSkeletonAssetVersion = 1;
	inline constexpr std::uint32_t kInvalidCookedSkeletonJointIndex = (std::numeric_limits<std::uint32_t>::max)();

	struct SPARKLE_ENGINE_API CookedSkeletonJointRecord
	{
		char name[64] = {};
		std::uint32_t sourceNodeIndex = (std::numeric_limits<std::uint32_t>::max)();
		std::uint32_t parentJointIndex = kInvalidCookedSkeletonJointIndex;
		DirectX::XMFLOAT4X4 inverseBindMatrix;
		DirectX::XMFLOAT4X4 bindPoseWorldTransform;
	};

	struct SPARKLE_ENGINE_API CookedSkeletonAssetHeader
	{
		CookedAssetHeader fileHeader{kCookedSkeletonAssetMagic, kCookedSkeletonAssetVersion};
		std::uint32_t jointCount = 0;
		std::uint32_t jointStride = sizeof(CookedSkeletonJointRecord);
		std::uint32_t flags = 0;
	};
}

static_assert(std::is_trivially_copyable_v<Assets::CookedSkeletonJointRecord>, "CookedSkeletonJointRecord must stay trivially copyable.");
static_assert(std::is_trivially_copyable_v<Assets::CookedSkeletonAssetHeader>, "CookedSkeletonAssetHeader must stay trivially copyable.");
