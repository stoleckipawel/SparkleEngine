#include "PCH.h"
#include "World/ECS/EntityRegistry.h"

#include <limits>

namespace ECS
{
	EntityId EntityRegistry::Create()
	{
		if (!CanMutateStructure())
		{
			return EntityId::Invalid();
		}

		if (!m_freeSlots.empty())
		{
			const EntityId::Slot slot = m_freeSlots.back();
			m_freeSlots.pop_back();
			EntitySlot& entry = m_slots[slot];
			entry.Alive = true;
			++m_liveCount;
			AdvanceStructureVersion();
			return EntityId(slot, entry.Generation);
		}

		if (m_slots.size() >= EntityId::InvalidSlot)
		{
			return EntityId::Invalid();
		}

		const EntityId::Slot slot = static_cast<EntityId::Slot>(m_slots.size());
		m_slots.push_back(EntitySlot{.Generation = 1, .Alive = true});
		++m_liveCount;
		AdvanceStructureVersion();
		return EntityId(slot, 1);
	}

	bool EntityRegistry::Destroy(EntityId entity)
	{
		if (!CanMutateStructure() || !IsAlive(entity))
		{
			return false;
		}

		m_componentTypes.RemoveEntity(entity);
		EntitySlot& entry = m_slots[entity.GetSlot()];
		entry.Alive = false;
		--m_liveCount;
		AdvanceStructureVersion();

		if (entry.Generation == (std::numeric_limits<EntityId::Generation>::max)())
		{
			entry.Retired = true;
			return true;
		}

		++entry.Generation;
		m_freeSlots.push_back(entity.GetSlot());
		return true;
	}

	bool EntityRegistry::IsAlive(EntityId entity) const noexcept
	{
		if (!entity.IsValid() || entity.GetSlot() >= m_slots.size())
		{
			return false;
		}
		const EntitySlot& entry = m_slots[entity.GetSlot()];
		return entry.Alive && !entry.Retired && entry.Generation == entity.GetGeneration();
	}

	bool EntityRegistry::Reserve(std::size_t entityCapacity)
	{
		if (!CanMutateStructure())
		{
			return false;
		}
		m_slots.reserve(entityCapacity);
		m_freeSlots.reserve(entityCapacity);
		return true;
	}

	bool EntityRegistry::Clear() noexcept
	{
		if (!CanMutateStructure())
		{
			return false;
		}
		m_componentTypes.Clear();
		m_freeSlots.clear();
		for (std::size_t slotIndex = m_slots.size(); slotIndex > 0; --slotIndex)
		{
			const EntityId::Slot slot = static_cast<EntityId::Slot>(slotIndex - 1);
			EntitySlot& entry = m_slots[slot];
			if (entry.Alive)
			{
				entry.Alive = false;
				if (entry.Generation == (std::numeric_limits<EntityId::Generation>::max)())
				{
					entry.Retired = true;
				}
				else
				{
					++entry.Generation;
				}
			}
			if (!entry.Retired)
			{
				m_freeSlots.push_back(slot);
			}
		}
		m_liveCount = 0;
		AdvanceStructureVersion();
		return true;
	}

	StructureFrozenEpoch EntityRegistry::FreezeStructure() noexcept
	{
		if (m_structureFrozen || m_frozenEpochGeneration == (std::numeric_limits<std::uint64_t>::max)())
		{
			return {};
		}
		++m_frozenEpochGeneration;
		m_structureFrozen = true;
		return StructureFrozenEpoch(*this, m_frozenEpochGeneration);
	}

	void EntityRegistry::ReleaseFrozenEpoch(std::uint64_t generation) noexcept
	{
		if (IsFrozenEpochCurrent(generation))
		{
			m_structureFrozen = false;
		}
	}
}
