#include "PCH.h"

#include "World/Resources/MorphWeightStorage.h"

#include <algorithm>
#include <limits>

namespace ECS
{
	AnimationOutputSlotHandle MorphWeightStorage::Add(std::span<const float> weights)
	{
		if (!m_freeSlots.empty())
		{
			const std::uint32_t slot = m_freeSlots.back();
			m_freeSlots.pop_back();
			Entry& entry = m_entries[slot];
			entry.Weights.assign(weights.begin(), weights.end());
			entry.Occupied = true;
			return {slot, entry.Generation};
		}
		if (m_entries.size() >= AnimationOutputSlotHandle{}.Slot)
			return {};
		const auto slot = static_cast<std::uint32_t>(m_entries.size());
		m_entries.push_back(Entry{.Weights = std::vector<float>(weights.begin(), weights.end()), .Occupied = true});
		return {slot, m_entries.back().Generation};
	}

	bool MorphWeightStorage::PrepareWriteSize(AnimationOutputSlotHandle handle, std::size_t weightCount)
	{
		if (!handle.IsValid() || handle.Slot >= m_entries.size())
			return false;
		Entry& entry = m_entries[handle.Slot];
		if (!entry.Occupied || entry.Generation != handle.Generation)
			return false;
		entry.Weights.resize(weightCount);
		return true;
	}

	bool MorphWeightStorage::Write(AnimationOutputSlotHandle handle, std::span<const float> weights) noexcept
	{
		if (!handle.IsValid() || handle.Slot >= m_entries.size())
			return false;
		Entry& entry = m_entries[handle.Slot];
		if (!entry.Occupied || entry.Generation != handle.Generation || entry.Weights.size() != weights.size())
			return false;
		std::copy(weights.begin(), weights.end(), entry.Weights.begin());
		return true;
	}

	std::span<const float> MorphWeightStorage::Read(AnimationOutputSlotHandle handle) const noexcept
	{
		if (!handle.IsValid() || handle.Slot >= m_entries.size())
			return {};
		const Entry& entry = m_entries[handle.Slot];
		return entry.Occupied && entry.Generation == handle.Generation ? std::span<const float>(entry.Weights) : std::span<const float>{};
	}

	bool MorphWeightStorage::Remove(AnimationOutputSlotHandle handle) noexcept
	{
		if (!handle.IsValid() || handle.Slot >= m_entries.size())
			return false;
		Entry& entry = m_entries[handle.Slot];
		if (!entry.Occupied || entry.Generation != handle.Generation)
			return false;
		entry.Weights.clear();
		entry.Occupied = false;
		if (entry.Generation != (std::numeric_limits<std::uint32_t>::max)())
		{
			++entry.Generation;
			m_freeSlots.push_back(handle.Slot);
		}
		return true;
	}

	void MorphWeightStorage::Clear() noexcept
	{
		m_entries.clear();
		m_freeSlots.clear();
	}
}
