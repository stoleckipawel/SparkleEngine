#include "PCH.h"

#include "World/Resources/AnimationClipResourceStore.h"

namespace ECS
{
	AnimationResourceHandle AnimationClipResourceStore::Add(AnimationClipResource&& clip)
	{
		if (m_entries.size() >= AnimationResourceHandle{}.Slot)
			return {};
		const auto slot = static_cast<std::uint32_t>(m_entries.size());
		m_entries.push_back(Entry{.Resource = std::move(clip)});
		return AnimationResourceHandle{slot, m_entries.back().Generation};
	}

	ResolvedAnimationClip AnimationClipResourceStore::Resolve(AnimationResourceHandle handle) const noexcept
	{
		if (!handle.IsValid() || handle.Slot >= m_entries.size())
			return {};
		const Entry& entry = m_entries[handle.Slot];
		return entry.Generation == handle.Generation
		           ? ResolvedAnimationClip{&entry.Resource, entry.Skeleton, entry.MorphChannelIndices, entry.TargetGeneration}
		           : ResolvedAnimationClip{};
	}

	bool AnimationClipResourceStore::ResolveTargets(
	    const SkeletonResourceStore& skeletons,
	    std::uint32_t targetGeneration) noexcept
	{
		if (targetGeneration == 0)
			return false;
		for (Entry& entry : m_entries)
		{
			entry.Skeleton = skeletons.Find(entry.Resource.targetSkeletonAssetId);
			entry.MorphChannelIndices.clear();
			entry.MorphChannelIndices.reserve(entry.Resource.channels.size());
			for (std::uint32_t index = 0; index < entry.Resource.channels.size(); ++index)
			{
				if (entry.Resource.channels[index].targetPath == Assets::CookedAnimationTargetPath::Weights)
					entry.MorphChannelIndices.push_back(index);
			}
			entry.TargetGeneration = targetGeneration;
		}
		return true;
	}

	void AnimationClipResourceStore::Clear() noexcept { m_entries.clear(); }
}
