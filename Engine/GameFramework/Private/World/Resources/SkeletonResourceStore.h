#pragma once

#include "Animation/AnimationEvaluationTypes.h"

#include <compare>
#include <cstdint>
#include <limits>
#include <unordered_map>

#include <vector>

struct SkeletonResourceHandle final
{
	std::uint32_t Slot = (std::numeric_limits<std::uint32_t>::max)();
	std::uint32_t Generation = 0;

	constexpr bool IsValid() const noexcept { return Slot != (std::numeric_limits<std::uint32_t>::max)() && Generation != 0; }
	constexpr auto operator<=>(const SkeletonResourceHandle&) const noexcept = default;
};

class SkeletonResourceStore final
{
public:
	void Append(std::vector<SkeletonResource>&& skeletons);
	SkeletonResourceHandle Find(Assets::CookedAssetId skeletonAssetId) const noexcept;
	ECS::SkeletonEvaluationData Resolve(SkeletonResourceHandle handle) const noexcept;
	std::size_t GetCount() const noexcept { return m_entries.size(); }

private:
	struct Entry final
	{
		SkeletonResource Resource;
		std::vector<ECS::AnimationJointTransform> BindLocalTransforms;
		std::vector<std::uint32_t> EvaluationOrder;
		std::uint32_t Generation = 1;
	};

	std::vector<Entry> m_entries;
	std::unordered_map<Assets::CookedAssetId, SkeletonResourceHandle> m_byAssetId;
};
