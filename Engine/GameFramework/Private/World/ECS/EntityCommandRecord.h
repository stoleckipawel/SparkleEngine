#pragma once

#include "World/ECS/EntityCommandTypes.h"
#include "World/ECS/EntityRegistry.h"

#include <memory>
#include <optional>
#include <utility>

namespace ECS::EntityCommandDetail
{
	struct EntityCommandTarget final
	{
		EntityId Entity;
		TemporaryEntityId Temporary;
		bool IsTemporary = false;
	};

	class ComponentCommandOperation
	{
	public:
		virtual ~ComponentCommandOperation() = default;
		virtual RuntimeComponentTypeId GetComponentType() const noexcept = 0;
		virtual EntityCommandStatus Apply(EntityRegistry& registry, EntityId entity, EntityCommandKind kind) = 0;
	};

	template <ComponentStorageCompatible T> class TypedComponentCommandOperation final : public ComponentCommandOperation
	{
	public:
		TypedComponentCommandOperation() = default;
		explicit TypedComponentCommandOperation(T value) :
		    m_value(std::move(value))
		{
		}

		RuntimeComponentTypeId GetComponentType() const noexcept override { return ComponentTypeRegistry::GetTypeId<T>(); }

		EntityCommandStatus Apply(EntityRegistry& registry, EntityId entity, EntityCommandKind kind) override
		{
			const bool present = registry.Get<T>(entity) != nullptr;
			switch (kind)
			{
				case EntityCommandKind::Add:
					if (present)
					{
						return EntityCommandStatus::ComponentAlreadyPresent;
					}
					return registry.Add(entity, std::move(*m_value)) ? EntityCommandStatus::Applied : EntityCommandStatus::CapacityExceeded;
				case EntityCommandKind::Remove:
					if (!present)
					{
						return EntityCommandStatus::ComponentMissing;
					}
					return registry.Remove<T>(entity) ? EntityCommandStatus::Applied : EntityCommandStatus::ComponentMissing;
				case EntityCommandKind::Replace:
					if (!present)
					{
						return EntityCommandStatus::ComponentMissing;
					}
					return registry.Replace(entity, std::move(*m_value)) ? EntityCommandStatus::Applied
					                                                     : EntityCommandStatus::ComponentMissing;
				case EntityCommandKind::Set:
					if (present)
					{
						return registry.Replace(entity, std::move(*m_value)) ? EntityCommandStatus::Applied
						                                                     : EntityCommandStatus::StaleTarget;
					}
					return registry.Add(entity, std::move(*m_value)) ? EntityCommandStatus::Applied : EntityCommandStatus::CapacityExceeded;
				case EntityCommandKind::Create:
				case EntityCommandKind::Destroy:
					return EntityCommandStatus::StaleTarget;
			}
			return EntityCommandStatus::StaleTarget;
		}

	private:
		std::optional<T> m_value;
	};

	struct EntityCommandRecord final
	{
		EntityCommandKey Key;
		EntityCommandKind Kind = EntityCommandKind::Create;
		EntityCommandTarget Target;
		std::unique_ptr<ComponentCommandOperation> ComponentOperation;
	};
}
