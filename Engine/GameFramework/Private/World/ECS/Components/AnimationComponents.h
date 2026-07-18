#pragma once

#include "GameFramework/Public/Assets/Cooked/CookedAssetCommon.h"

#include <compare>
#include <cstdint>
#include <limits>

namespace ECS
{
	struct AnimationResourceHandle final
	{
		std::uint32_t Slot = (std::numeric_limits<std::uint32_t>::max)();
		std::uint32_t Generation = 0;

		constexpr bool IsValid() const noexcept
		{
			return Slot != (std::numeric_limits<std::uint32_t>::max)() && Generation != 0;
		}
		constexpr auto operator<=>(const AnimationResourceHandle&) const noexcept = default;
	};

	struct SceneStateHandle final
	{
		std::uint32_t Slot = (std::numeric_limits<std::uint32_t>::max)();
		std::uint32_t Generation = 0;

		constexpr bool IsValid() const noexcept { return Slot != (std::numeric_limits<std::uint32_t>::max)() && Generation != 0; }
		constexpr auto operator<=>(const SceneStateHandle&) const noexcept = default;
	};

	struct AnimationState final
	{
		AnimationResourceHandle Resource;
		Assets::CookedAssetId AnimationAssetId = Assets::InvalidCookedAssetId;
		float TimeSeconds = 0.0f;
		float PlaybackRate = 1.0f;
		bool Playing = true;
		bool Looping = true;
	};

	struct MorphState final
	{
		SceneStateHandle Weights;
	};

	struct SkinningState final
	{
		SceneStateHandle Pose;
		Assets::CookedAssetId SkeletonAssetId = Assets::InvalidCookedAssetId;
	};
}
