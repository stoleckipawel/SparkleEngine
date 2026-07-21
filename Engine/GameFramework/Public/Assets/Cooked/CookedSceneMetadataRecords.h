#pragma once

#include "GameFramework/Public/Assets/Cooked/CookedAssetCommon.h"
#include "GameFramework/Public/GameFrameworkAPI.h"

#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace Assets
{
	enum class CookedSceneFeatureFlags : std::uint32_t
	{
		None = 0,
		Cameras = 1u << 0u,
		Lights = 1u << 1u,
		Skeletons = 1u << 2u,
		Animations = 1u << 3u,
		SkinnedMeshes = 1u << 4u,
		MorphTargets = 1u << 5u,
		MaterialVariants = 1u << 6u,
		AuthoredMeshInstancing = 1u << 7u,
	};

	constexpr std::uint32_t ToCookedSceneFeatureFlagMask(CookedSceneFeatureFlags flags) noexcept
	{
		return static_cast<std::uint32_t>(flags);
	}

	struct SPARKLE_ENGINE_API CookedSceneSkeletonRef
	{
		CookedAssetId skeletonAssetId = InvalidCookedAssetId;
		std::uint32_t sourceSkinIndex = 0;
		std::uint32_t flags = 0;
	};

	struct SPARKLE_ENGINE_API CookedAnimationReference
	{
		CookedAssetId animationAssetId = InvalidCookedAssetId;
		std::uint32_t sourceAnimationIndex = 0;
		std::uint32_t flags = 0;
	};

	inline constexpr std::size_t kCookedSceneMaterialVariantNameCapacity = 64;

	struct SPARKLE_ENGINE_API CookedSceneMaterialVariantRecord
	{
		char name[kCookedSceneMaterialVariantNameCapacity] = {};
		std::uint32_t sourceVariantIndex = 0;
	};

	struct SPARKLE_ENGINE_API CookedSceneMaterialVariantMappingRecord
	{
		std::uint32_t meshAssetIndex = 0;
		std::uint32_t variantIndex = 0;
		std::uint32_t materialAssetIndex = 0;
	};
}  // namespace Assets

static_assert(std::is_trivially_copyable_v<Assets::CookedSceneSkeletonRef>, "CookedSceneSkeletonRef must stay trivially copyable.");
static_assert(std::is_trivially_copyable_v<Assets::CookedAnimationReference>, "CookedAnimationReference must stay trivially copyable.");
static_assert(
    std::is_trivially_copyable_v<Assets::CookedSceneMaterialVariantRecord>,
    "CookedSceneMaterialVariantRecord must stay trivially copyable.");
static_assert(
    std::is_trivially_copyable_v<Assets::CookedSceneMaterialVariantMappingRecord>,
    "CookedSceneMaterialVariantMappingRecord must stay trivially copyable.");
