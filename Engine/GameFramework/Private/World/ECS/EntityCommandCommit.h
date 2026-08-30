#pragma once

#include "World/ECS/EntityCommandBuffer.h"

#include <span>

namespace ECS
{
	class EntityCommandCommit final
	{
	public:
		static EntityCommandCommitResult Apply(
		    EntityRegistry& registry,
		    std::span<EntityCommandBuffer* const> buffers,
		    EntityCommandConflictPolicy conflictPolicy);
	};
}
