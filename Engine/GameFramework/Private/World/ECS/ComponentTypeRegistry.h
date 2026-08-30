#pragma once

#include "World/ECS/ComponentStorage.h"

#include <cstdint>
#include <memory>
#include <unordered_map>

namespace ECS
{
	struct RuntimeComponentTypeId final
	{
		std::uint32_t Value = 0;
		constexpr auto operator<=>(const RuntimeComponentTypeId&) const noexcept = default;
	};

	class ComponentTypeRegistry final
	{
	public:
		template <typename T> static RuntimeComponentTypeId GetTypeId() noexcept { return GetRuntimeComponentTypeId<T>(); }

		template <ComponentStorageCompatible T> ComponentStorage<T>& GetOrCreate()
		{
			const RuntimeComponentTypeId typeId = GetRuntimeComponentTypeId<T>();
			auto existing = m_storages.find(typeId.Value);
			if (existing != m_storages.end())
			{
				return *static_cast<ComponentStorage<T>*>(existing->second.get());
			}

			auto storage = std::make_unique<ComponentStorage<T>>();
			ComponentStorage<T>* result = storage.get();
			m_storages.emplace(typeId.Value, std::move(storage));
			return *result;
		}

		template <ComponentStorageCompatible T> ComponentStorage<T>* Find() noexcept
		{
			const auto existing = m_storages.find(GetRuntimeComponentTypeId<T>().Value);
			return existing == m_storages.end() ? nullptr : static_cast<ComponentStorage<T>*>(existing->second.get());
		}

		template <ComponentStorageCompatible T> const ComponentStorage<T>* Find() const noexcept
		{
			const auto existing = m_storages.find(GetRuntimeComponentTypeId<T>().Value);
			return existing == m_storages.end() ? nullptr : static_cast<const ComponentStorage<T>*>(existing->second.get());
		}

		void RemoveEntity(EntityId entity)
		{
			for (auto& [typeId, storage] : m_storages)
			{
				static_cast<void>(typeId);
				storage->Remove(entity);
			}
		}

		void Clear() noexcept { m_storages.clear(); }

	private:
		static RuntimeComponentTypeId AllocateRuntimeComponentTypeId() noexcept
		{
			static std::uint32_t nextTypeId = 0;
			return RuntimeComponentTypeId{nextTypeId++};
		}

		template <typename T> static RuntimeComponentTypeId GetRuntimeComponentTypeId() noexcept
		{
			static const RuntimeComponentTypeId typeId = AllocateRuntimeComponentTypeId();
			return typeId;
		}

		std::unordered_map<std::uint32_t, std::unique_ptr<ComponentStorageBase>> m_storages;
	};
}
