#include "PCH.h"
#include "World/Systems/CameraDerivedStateEvaluationSystem.h"

#include "World/ECS/Components/RenderingComponents.h"
#include "World/ECS/Components/TransformComponents.h"
#include "World/ECS/EntityRegistry.h"

#include <cmath>

namespace ECS
{
	void CameraDerivedStateEvaluationSystem::Evaluate(
	    EntityRegistry& registry,
	    std::span<const EntityId> dirtyEntities) noexcept
	{
		for (EntityId entity : dirtyEntities)
		{
			if (registry.Get<Camera>(entity) == nullptr)
			{
				continue;
			}
			const LocalTransform* local = registry.Get<LocalTransform>(entity);
			if (local == nullptr)
			{
				continue;
			}
			const DirectX::XMVECTOR forward = DirectX::XMVector3Rotate(
			    DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f),
			    DirectX::XMLoadFloat4(&local->Rotation));
			CameraDerivedState derived;
			DirectX::XMStoreFloat3(&derived.Direction, DirectX::XMVector3Normalize(forward));
			registry.Replace(entity, derived);
		}
	}
}
