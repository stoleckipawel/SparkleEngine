#pragma once

#include "GameFramework/Public/World/EntityId.h"

#include <span>

namespace ECS
{
	class EntityRegistry;

	class CameraDerivedStateEvaluationSystem final
	{
	  public:
		static void Evaluate(EntityRegistry& registry, std::span<const EntityId> dirtyEntities) noexcept;
	};
}
