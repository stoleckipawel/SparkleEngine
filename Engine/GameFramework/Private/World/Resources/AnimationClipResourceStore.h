#pragma once

#include "Animation/AnimationClipResource.h"
#include "World/ECS/Components/AnimationComponents.h"
#include "World/Resources/SkeletonResourceStore.h"

#include <cstdint>
#include <span>
#include <vector>

namespace ECS
{
	struct ResolvedAnimationClip final
	{
		const AnimationClipResource* Resource = nullptr;
		SkeletonResourceHandle Skeleton;
		std::span<const std::uint32_t> MorphChannelIndices;
		std::uint32_t TargetGeneration = 0;

		bool IsValid() const noexcept { return Resource != nullptr && TargetGeneration != 0; }
	};

	class AnimationClipResourceStore final
	{
	  public:
		AnimationResourceHandle Add(AnimationClipResource&& clip);
		ResolvedAnimationClip Resolve(AnimationResourceHandle handle) const noexcept;
		bool ResolveTargets(const SkeletonResourceStore& skeletons, std::uint32_t targetGeneration) noexcept;
		void Clear() noexcept;

	  private:
		struct Entry final
		{
			AnimationClipResource Resource;
			SkeletonResourceHandle Skeleton;
			std::vector<std::uint32_t> MorphChannelIndices;
			std::uint32_t Generation = 1;
			std::uint32_t TargetGeneration = 0;
		};

		std::vector<Entry> m_entries;
	};
}
