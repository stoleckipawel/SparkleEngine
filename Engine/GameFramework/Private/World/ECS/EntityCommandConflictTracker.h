#pragma once

#include "World/ECS/EntityCommandTypes.h"

#include <cstddef>
#include <cstdint>
#include <unordered_map>
#include <vector>

namespace ECS::EntityCommandDetail
{
	class EntityCommandConflictTracker final
	{
	public:
		void Reserve(std::size_t commandCount);
		bool TryClaim(
		    EntityId entity,
		    EntityCommandBufferId bufferId,
		    EntityCommandKind kind,
		    RuntimeComponentTypeId componentType,
		    bool hasComponentType);

	private:
		struct ComponentOwner final
		{
			RuntimeComponentTypeId Type;
			EntityCommandBufferId Buffer;
		};

		struct EntityConflictState final
		{
			std::vector<ComponentOwner> ComponentOwners;
			EntityCommandBufferId EntityWideOwner;
			bool HasEntityWideOwner = false;
		};

		static std::uint64_t PackEntity(EntityId entity) noexcept;
		static const ComponentOwner* FindComponentOwner(const EntityConflictState& state, RuntimeComponentTypeId type) noexcept;

		std::unordered_map<std::uint64_t, EntityConflictState> m_entities;
	};
}
