#pragma once

#include "GameFramework/Public/World/EntityId.h"

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>
#include <type_traits>
#include <utility>
#include <vector>

namespace ECS
{
	class EntityRegistry;
	template <typename... AccessSpecs> class Query;

	struct ComponentStorageVersion final
	{
		std::uint64_t Structure = 0;
		std::uint64_t Content = 0;

		constexpr auto operator<=>(const ComponentStorageVersion&) const noexcept = default;
	};

	struct ComponentQueryVersion final
	{
		ComponentStorageVersion Storage;

		constexpr auto operator<=>(const ComponentQueryVersion&) const noexcept = default;
	};

	class ComponentStorageBase
	{
	  public:
		virtual ~ComponentStorageBase() = default;
		virtual void Remove(EntityId entity) = 0;
	};

	template <typename T>
	concept ComponentStorageCompatible = std::copy_constructible<T> && std::is_copy_assignable_v<T> &&
	                                     std::is_nothrow_move_constructible_v<T> && std::is_nothrow_move_assignable_v<T>;

	template <ComponentStorageCompatible T> class ComponentStorage final : public ComponentStorageBase
	{
	  public:
		bool Add(EntityId entity, T component)
		{
			if (!entity.IsValid() || IsSlotOccupied(entity.GetSlot()) || m_entities.size() >= InvalidDenseIndex)
			{
				return false;
			}

			PrepareCapacity(m_entities.size() + 1, entity.GetSlot());
			const std::uint32_t denseIndex = static_cast<std::uint32_t>(m_entities.size());
			m_entities.push_back(entity);
			m_components.push_back(std::move(component));
			m_sparse[entity.GetSlot()] = denseIndex;
			AdvanceStructureVersion();
			return true;
		}

		bool AddRange(std::span<const EntityId> entities, std::span<T> components)
		{
			if (entities.size() != components.size() || entities.size() > InvalidDenseIndex - m_entities.size() || !CanAddRange(entities))
			{
				return false;
			}
			if (entities.empty())
			{
				return true;
			}

			EntityId::Slot maximumSlot = 0;
			for (EntityId entity : entities)
			{
				maximumSlot = (std::max) (maximumSlot, entity.GetSlot());
			}
			PrepareCapacity(m_entities.size() + entities.size(), maximumSlot);
			for (std::size_t index = 0; index < entities.size(); ++index)
			{
				const EntityId entity = entities[index];
				m_sparse[entity.GetSlot()] = static_cast<std::uint32_t>(m_entities.size());
				m_entities.push_back(entity);
				m_components.push_back(std::move(components[index]));
			}
			if (!entities.empty())
			{
				AdvanceStructureVersion();
			}
			return true;
		}

		void Remove(EntityId entity) override
		{
			const std::uint32_t denseIndex = FindDenseIndex(entity);
			if (denseIndex == InvalidDenseIndex)
			{
				return;
			}

			const std::uint32_t lastIndex = static_cast<std::uint32_t>(m_entities.size() - 1);
			if (denseIndex != lastIndex)
			{
				m_entities[denseIndex] = m_entities[lastIndex];
				m_components[denseIndex] = std::move(m_components[lastIndex]);
				m_sparse[m_entities[denseIndex].GetSlot()] = denseIndex;
			}

			m_sparse[entity.GetSlot()] = InvalidDenseIndex;
			m_entities.pop_back();
			m_components.pop_back();
			AdvanceStructureVersion();
		}

		bool Replace(EntityId entity, T component)
		{
			const std::uint32_t denseIndex = FindDenseIndex(entity);
			if (denseIndex == InvalidDenseIndex)
			{
				return false;
			}
			m_components[denseIndex] = std::move(component);
			++m_version.Content;
			return true;
		}

		bool Contains(EntityId entity) const noexcept { return FindDenseIndex(entity) != InvalidDenseIndex; }

		const T* Get(EntityId entity) const noexcept
		{
			const std::uint32_t denseIndex = FindDenseIndex(entity);
			return denseIndex == InvalidDenseIndex ? nullptr : &m_components[denseIndex];
		}

		void Reserve(std::size_t capacity)
		{
			m_entities.reserve(capacity);
			m_components.reserve(capacity);
		}

		std::span<const EntityId> GetEntities() const noexcept { return m_entities; }
		std::span<const T> GetComponents() const noexcept { return m_components; }
		ComponentStorageVersion GetVersion() const noexcept { return m_version; }
		ComponentQueryVersion CaptureQueryVersion() const noexcept { return ComponentQueryVersion{m_version}; }

	  private:
		friend class EntityRegistry;
		template <typename... AccessSpecs> friend class Query;

		static constexpr std::uint32_t InvalidDenseIndex = (std::numeric_limits<std::uint32_t>::max)();

		std::uint32_t FindDenseIndex(EntityId entity) const noexcept
		{
			if (!entity.IsValid() || entity.GetSlot() >= m_sparse.size())
			{
				return InvalidDenseIndex;
			}
			const std::uint32_t denseIndex = m_sparse[entity.GetSlot()];
			return denseIndex < m_entities.size() && m_entities[denseIndex] == entity ? denseIndex : InvalidDenseIndex;
		}

		T* GetMutable(EntityId entity) noexcept
		{
			const std::uint32_t denseIndex = FindDenseIndex(entity);
			return denseIndex == InvalidDenseIndex ? nullptr : &m_components[denseIndex];
		}

		void MarkContentChanged() noexcept { ++m_version.Content; }

		bool IsSlotOccupied(EntityId::Slot slot) const noexcept { return slot < m_sparse.size() && m_sparse[slot] < m_entities.size(); }

		bool CanAddRange(std::span<const EntityId> entities) const
		{
			std::vector<EntityId::Slot> slots;
			slots.reserve(entities.size());
			for (EntityId entity : entities)
			{
				if (!entity.IsValid() || IsSlotOccupied(entity.GetSlot()))
				{
					return false;
				}
				slots.push_back(entity.GetSlot());
			}
			std::sort(slots.begin(), slots.end());
			return std::adjacent_find(slots.begin(), slots.end()) == slots.end();
		}

		void PrepareCapacity(std::size_t denseCapacity, EntityId::Slot maximumSlot)
		{
			Reserve(denseCapacity);
			if (maximumSlot >= m_sparse.size())
			{
				m_sparse.resize(static_cast<std::size_t>(maximumSlot) + 1, InvalidDenseIndex);
			}
		}

		void AdvanceStructureVersion() noexcept
		{
			++m_version.Structure;
			++m_version.Content;
		}

		std::vector<std::uint32_t> m_sparse;
		std::vector<EntityId> m_entities;
		std::vector<T> m_components;
		ComponentStorageVersion m_version;
	};
}
