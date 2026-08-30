#pragma once

#include "GameFramework/Public/Rendering/RenderObjectId.h"
#include "GameFramework/Public/World/EntityId.h"

#include <cstdint>
#include <map>

namespace ECS
{
	class RenderObjectIdentityMap final
	{
	public:
		void BeginScene() noexcept;
		RenderObjectId Resolve(EntityId entity);

	private:
		std::map<EntityId, RenderObjectId> m_objects;
		std::uint32_t m_nextValue = 0;
		std::uint32_t m_generation = 0;
	};
}
