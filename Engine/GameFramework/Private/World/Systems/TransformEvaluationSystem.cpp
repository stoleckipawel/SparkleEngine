#include "PCH.h"
#include "World/Systems/TransformEvaluationSystem.h"

#include "World/ECS/Components/TransformComponents.h"
#include "World/ECS/EntityRegistry.h"
#include "World/WorldTransformConversion.h"

namespace ECS
{
	void TransformEvaluationSystem::Evaluate(EntityRegistry& registry, std::span<const EntityId> dirtyEntities) noexcept
	{
		for (EntityId entity : dirtyEntities)
		{
			const LocalTransform* local = registry.Get<LocalTransform>(entity);
			if (local != nullptr)
			{
				registry.Replace(entity, WorldTransformConversion::BuildWorld(*local));
			}
		}
	}
}
