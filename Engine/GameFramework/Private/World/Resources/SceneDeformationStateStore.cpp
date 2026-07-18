#include "PCH.h"
#include "World/Resources/SceneDeformationStateStore.h"

#include <limits>

namespace ECS
{
	SceneStateHandle SceneDeformationStateStore::AddMorphWeights(std::span<const float> weights)
	{
		if (!m_freeSlots.empty())
		{
			const std::uint32_t slot = m_freeSlots.back();
			m_freeSlots.pop_back();
			Entry& entry = m_entries[slot];
			entry.Weights.assign(weights.begin(), weights.end());
			entry.Occupied = true;
			return SceneStateHandle{slot, entry.Generation};
		}
		if (m_entries.size() >= SceneStateHandle{}.Slot)
		{
			return {};
		}
		const auto slot = static_cast<std::uint32_t>(m_entries.size());
		m_entries.push_back(Entry{.Weights = std::vector<float>(weights.begin(), weights.end()), .Occupied = true});
		return SceneStateHandle{slot, m_entries.back().Generation};
	}

	bool SceneDeformationStateStore::WriteMorphWeights(SceneStateHandle handle, std::span<const float> weights)
	{
		if (!handle.IsValid() || handle.Slot >= m_entries.size())
		{
			return false;
		}
		Entry& entry = m_entries[handle.Slot];
		if (!entry.Occupied || entry.Generation != handle.Generation)
		{
			return false;
		}
		entry.Weights.assign(weights.begin(), weights.end());
		return true;
	}

	std::span<const float> SceneDeformationStateStore::ReadMorphWeights(SceneStateHandle handle) const noexcept
	{
		if (!handle.IsValid() || handle.Slot >= m_entries.size())
		{
			return {};
		}
		const Entry& entry = m_entries[handle.Slot];
		return entry.Occupied && entry.Generation == handle.Generation ? std::span<const float>(entry.Weights) : std::span<const float>{};
	}

	bool SceneDeformationStateStore::Remove(SceneStateHandle handle) noexcept
	{
		if (!handle.IsValid() || handle.Slot >= m_entries.size())
		{
			return false;
		}
		Entry& entry = m_entries[handle.Slot];
		if (!entry.Occupied || entry.Generation != handle.Generation)
		{
			return false;
		}
		entry.Weights.clear();
		entry.Occupied = false;
		if (entry.Generation != (std::numeric_limits<std::uint32_t>::max)())
		{
			++entry.Generation;
			m_freeSlots.push_back(handle.Slot);
		}
		return true;
	}

	void SceneDeformationStateStore::Clear() noexcept
	{
		m_entries.clear();
		m_freeSlots.clear();
	}
}
