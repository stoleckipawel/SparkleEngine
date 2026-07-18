#pragma once

#include "GameFramework/Public/World/EntityId.h"
#include "World/ECS/ComponentTypeRegistry.h"

#include <cstddef>
#include <span>
#include <utility>
#include <vector>

namespace ECS
{
	class EntityRegistry final
	{
	  public:
		EntityId Create();
		bool Destroy(EntityId entity);
		bool IsAlive(EntityId entity) const noexcept;
		void Reserve(std::size_t entityCapacity);
		std::size_t GetLiveCount() const noexcept { return m_liveCount; }

		template <ComponentStorageCompatible T> bool Add(EntityId entity, T component)
		{
			return IsAlive(entity) && m_componentTypes.GetOrCreate<T>().Add(entity, std::move(component));
		}

		template <ComponentStorageCompatible T> bool AddRange(std::span<const EntityId> entities, std::span<T> components)
		{
			if (entities.size() != components.size())
			{
				return false;
			}
			for (EntityId entity : entities)
			{
				if (!IsAlive(entity))
				{
					return false;
				}
			}
			return m_componentTypes.GetOrCreate<T>().AddRange(entities, components);
		}

		template <ComponentStorageCompatible T> bool Remove(EntityId entity)
		{
			ComponentStorage<T>* storage = m_componentTypes.Find<T>();
			if (!IsAlive(entity) || storage == nullptr || !storage->Contains(entity))
			{
				return false;
			}
			storage->Remove(entity);
			return true;
		}

		template <ComponentStorageCompatible T> bool Replace(EntityId entity, T component)
		{
			ComponentStorage<T>* storage = m_componentTypes.Find<T>();
			return IsAlive(entity) && storage != nullptr && storage->Replace(entity, std::move(component));
		}

		template <ComponentStorageCompatible T> const T* Get(EntityId entity) const noexcept
		{
			const ComponentStorage<T>* storage = m_componentTypes.Find<T>();
			return IsAlive(entity) && storage != nullptr ? storage->Get(entity) : nullptr;
		}

		template <ComponentStorageCompatible T> const ComponentStorage<T>* FindStorage() const noexcept
		{
			return m_componentTypes.Find<T>();
		}

	  private:
		struct EntitySlot final
		{
			EntityId::Generation Generation = 1;
			bool Alive = false;
			bool Retired = false;
		};

		std::vector<EntitySlot> m_slots;
		std::vector<EntityId::Slot> m_freeSlots;
		ComponentTypeRegistry m_componentTypes;
		std::size_t m_liveCount = 0;
	};
}
