#include "PCH.h"
#include "World/Resources/SceneAnimationResources.h"

namespace ECS
{
	AnimationResourceHandle SceneAnimationResources::Add(SceneAnimationClipDesc&& clip)
	{
		if (m_entries.size() >= AnimationResourceHandle{}.Slot)
		{
			return {};
		}
		const auto slot = static_cast<std::uint32_t>(m_entries.size());
		m_entries.push_back(Entry{.Clip = std::move(clip)});
		return AnimationResourceHandle{slot, m_entries.back().Generation};
	}

	const SceneAnimationClipDesc* SceneAnimationResources::Resolve(AnimationResourceHandle handle) const noexcept
	{
		if (!handle.IsValid() || handle.Slot >= m_entries.size())
		{
			return nullptr;
		}
		const Entry& entry = m_entries[handle.Slot];
		return entry.Generation == handle.Generation ? &entry.Clip : nullptr;
	}

	void SceneAnimationResources::Clear() noexcept
	{
		m_entries.clear();
		m_derivedOutput.Reset();
	}
}
