#include "PCH.h"
#include "World/ECS/EntityCommandConflictTracker.h"

#include <algorithm>

namespace ECS::EntityCommandDetail
{
	void EntityCommandConflictTracker::Reserve(std::size_t commandCount)
	{
		m_entities.reserve(commandCount);
	}

	bool EntityCommandConflictTracker::TryClaim(
	    EntityId entity,
	    EntityCommandBufferId bufferId,
	    EntityCommandKind kind,
	    RuntimeComponentTypeId componentType,
	    bool hasComponentType)
	{
		EntityConflictState& state = m_entities[PackEntity(entity)];
		if (kind == EntityCommandKind::Destroy)
		{
			if ((state.HasEntityWideOwner && state.EntityWideOwner != bufferId) || std::any_of(
			                                                                           state.ComponentOwners.begin(),
			                                                                           state.ComponentOwners.end(),
			                                                                           [&](const ComponentOwner& owner)
			                                                                           {
				                                                                           return owner.Buffer != bufferId;
			                                                                           }))
			{
				return false;
			}
			state.EntityWideOwner = bufferId;
			state.HasEntityWideOwner = true;
			return true;
		}

		if (!hasComponentType)
		{
			return true;
		}
		if (state.HasEntityWideOwner && state.EntityWideOwner != bufferId)
		{
			return false;
		}
		const ComponentOwner* componentOwner = FindComponentOwner(state, componentType);
		if (componentOwner != nullptr)
		{
			return componentOwner->Buffer == bufferId;
		}
		state.ComponentOwners.push_back({componentType, bufferId});
		return true;
	}

	std::uint64_t EntityCommandConflictTracker::PackEntity(EntityId entity) noexcept
	{
		return (static_cast<std::uint64_t>(entity.GetGeneration()) << 32u) | entity.GetSlot();
	}

	const EntityCommandConflictTracker::ComponentOwner* EntityCommandConflictTracker::FindComponentOwner(
	    const EntityConflictState& state,
	    RuntimeComponentTypeId type) noexcept
	{
		const auto owner = std::find_if(
		    state.ComponentOwners.begin(),
		    state.ComponentOwners.end(),
		    [&](const ComponentOwner& candidate)
		    {
			    return candidate.Type == type;
		    });
		return owner == state.ComponentOwners.end() ? nullptr : &*owner;
	}
}
